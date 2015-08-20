#include <kernel/process.h>
#include <kernel/klib.h>
#include <kernel/idt.h>
#include <kernel/gdt.h>
#include <mm/slab.h>

#define SYSCALL_INT_NUM 0x80

static struct kmem_cache *proc_cache;

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
    return slab_alloc(proc_cache);
}

void proc_free(struct process *proc)
{
    slab_free(proc_cache, proc);
}
