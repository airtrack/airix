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
static pid_t pid;

static pid_t generate_pid()
{
    return ++pid;
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
    if (proc) memset(proc, 0, sizeof(*proc));
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
            struct page_table *page_table =
                vmm_unmap_page_table_index(page_dir, pde, 0);

            if (page_table)
            {
                for (uint32_t pte = 0; pte < NUM_PTE; ++pte)
                {
                    physical_addr_t paddr =
                        vmm_unmap_page_index(page_table, pte, 0);
                    if (paddr != 0)
                        pmm_free_page_address(paddr);
                }

                vmm_free_page_table(page_table);
            }
        }

        vmm_free_vaddr_space(page_dir);
    }

    slab_free(proc_cache, proc);
}

static void alloc_proc_stacks(struct process *proc)
{
    physical_addr_t kstack = pmm_alloc_page_address();
    physical_addr_t ustack = pmm_alloc_page_address();

    if (kstack == 0 || ustack == 0)
        panic("Out of memory: alloc process stacks fail!");

    proc->kernel_stack = PROC_KERNEL_STACK;
    proc->user_stack = PROC_USER_STACK;

    /* Map kernel stack and user stack */
    vmm_map(proc->page_dir, (void *)(PROC_KERNEL_STACK - PAGE_SIZE),
            kstack, VMM_WRITABLE);
    vmm_map(proc->page_dir, (void *)(PROC_USER_STACK - PAGE_SIZE),
            ustack, VMM_WRITABLE | VMM_USER);
}

bool proc_exec(const char *elf, size_t size)
{
    struct process *proc = proc_alloc();

    if (!proc)
        return false;

    /* Prepare virtual address space and map kernel space */
    proc->page_dir = vmm_alloc_vaddr_space();
    pg_copy_kernel_space(proc->page_dir);

    /* Load program into process */
    if (!elf_load_program(elf, size, proc))
    {
        proc_free(proc);
        return false;
    }

    proc->pid = generate_pid();

    /* Prepare kernel stack and user stack */
    alloc_proc_stacks(proc);

    /* Schedule running process */
    sched_process(proc);
    return true;
}
