#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/base.h>
#include <kernel/klib.h>
#include <kernel/elf.h>
#include <kernel/idt.h>
#include <kernel/gdt.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <mm/paging.h>
#include <string.h>

/*
 * Addresses of process's stacks.
 * Kernel stack address and user stack address should not be in the
 * same page directory entry.
 */
#define PROC_KERNEL_STACK (KERNEL_BASE - 16 * PAGE_SIZE)
#define PROC_USER_STACK (KERNEL_BASE - 1024 * PAGE_SIZE)

/* System call INT number */
#define SYSCALL_INT_NUM 0x80

static struct kmem_cache *proc_cache;

/* PID bitmap */
static char pid_map[PROC_MAX_NUM / 8];

/* PID generator */
static pid_t pid_gen;

static pid_t alloc_pid()
{
    for (uint32_t i = 0; i < PROC_MAX_NUM; ++i)
    {
        pid_t pid = pid_gen;
        pid_gen = (pid_gen + 1) % PROC_MAX_NUM;

        /* Check if pid is avail */
        if (!(pid_map[pid / 8] & (1 << (pid % 8))))
        {
            pid_map[pid / 8] |= (1 << (pid % 8));
            return pid;
        }
    }

    return -1;
}

static void free_pid(pid_t pid)
{
    pid_map[pid / 8] &= ~(1 << (pid % 8));
}

void proc_initialize()
{
    /* Prepare syscall for user process */
    idt_set_entry(SYSCALL_INT_NUM, KERNEL_CODE_SELECTOR,
                  syscall_entry, IDT_TYPE_INT, DPL_3);

    proc_cache = slab_create_kmem_cache(
        sizeof(struct process), sizeof(void *));
}

struct process * proc_alloc()
{
    struct process *proc = slab_alloc(proc_cache);
    if (!proc)
        return NULL;

    memset(proc, 0, sizeof(*proc));
    proc->pid = alloc_pid();

    /* Alloc PID fail */
    if (proc->pid == -1)
    {
        proc_free(proc);
        return NULL;
    }

    return proc;
}

void proc_free(struct process *proc)
{
    /* Free virtual address space */
    if (proc->page_dir)
    {
        struct page_directory *page_dir = proc->page_dir;

        /* Free all user space memory pages */
        for (uint32_t pde = 0; pde < KERNEL_BASE / (NUM_PTE * PAGE_SIZE); ++pde)
        {
            struct page_table *page_tab =
                vmm_unmap_page_table_index(page_dir, pde, 0);

            if (page_tab)
            {
                for (uint32_t pte = 0; pte < NUM_PTE; ++pte)
                {
                    physical_addr_t paddr =
                        vmm_unmap_page_index(page_tab, pte, 0);
                    if (paddr != 0)
                        pmm_free_page_address(paddr);
                }

                vmm_free_page_table(page_tab);
            }
        }

        vmm_free_vaddr_space(page_dir);
    }

    /* Release PID */
    if (proc->pid != -1)
        free_pid(proc->pid);

    slab_free(proc_cache, proc);
}

static bool alloc_proc_stacks(struct process *proc)
{
    physical_addr_t stack = pmm_alloc_page_address();
    if (!stack) return false;

    /* Map kernel stack */
    if (!vmm_map(proc->page_dir, (void *)(PROC_KERNEL_STACK - PAGE_SIZE),
                 stack, VMM_WRITABLE))
    {
        pmm_free_page_address(stack);
        return false;
    }

    stack = pmm_alloc_page_address();
    if (!stack) return false;

    /* Map user stack */
    if (!vmm_map(proc->page_dir, (void *)(PROC_USER_STACK - PAGE_SIZE),
                 stack, VMM_WRITABLE | VMM_USER))
    {
        pmm_free_page_address(stack);
        return false;
    }

    /* Setup addresses */
    proc->kernel_stack = PROC_KERNEL_STACK;
    proc->user_stack = PROC_USER_STACK;
    return true;
}

static bool init_proc_from_elf(struct process *proc,
                               const char *elf, size_t size)
{
    /* Prepare virtual address space */
    proc->page_dir = vmm_alloc_vaddr_space();

    if (!proc->page_dir)
        return false;

    /* Load program into process */
    if (!elf_load_program(elf, size, proc))
        return false;

    /* Prepare kernel stack and user stack */
    if (!alloc_proc_stacks(proc))
        return false;

    pg_copy_kernel_space(proc->page_dir);
    return true;
}

bool proc_exec(const char *elf, size_t size)
{
    struct process *proc = proc_alloc();

    if (!proc)
        return false;

    if (!init_proc_from_elf(proc, elf, size))
    {
        proc_free(proc);
        return false;
    }

    /* Schedule running process */
    sched_process(proc);
    return true;
}
