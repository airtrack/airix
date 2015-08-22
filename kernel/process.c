#include <kernel/process.h>
#include <kernel/klib.h>
#include <kernel/elf.h>
#include <kernel/idt.h>
#include <kernel/gdt.h>
#include <mm/vmm.h>
#include <mm/slab.h>
#include <mm/paging.h>
#include <string.h>

#define SYSCALL_INT_NUM 0x80

static struct kmem_cache *proc_cache;
static pid_t pid;

void proc_initialize()
{
    /* Prepare syscall for user process */
    idt_set_entry(SYSCALL_INT_NUM, KERNEL_CODE_SELECTOR,
                  syscall_entry, IDT_TYPE_INT, DPL_0);

    proc_cache = slab_create_kmem_cache(
        sizeof(struct process), sizeof(void *));
}

struct process * proc_alloc()
{
    struct process *proc = slab_alloc(proc_cache);
    memset(proc, 0, sizeof(*proc));
    return proc;
}

void proc_free(struct process *proc)
{
    /* TODO: free page directory and page tables */
    slab_free(proc_cache, proc);
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

    /* Generate a pid */
    proc->pid = ++pid;

    /* TODO: prepare kernel stack and user stack */

    /* TODO: scheduler execute process */

    return true;
}
