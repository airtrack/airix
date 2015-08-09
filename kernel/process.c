#include <kernel/process.h>
#include <mm/slab.h>

static struct kmem_cache *proc_cache;

void proc_initialize()
{
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
