#include <kernel/klib.h>

static struct kernel_idle idle_head = { NULL, NULL, &idle_head, &idle_head };

void register_kernel_idle(struct kernel_idle *idle)
{
    idle->prev = idle_head.prev;
    idle->next = &idle_head;
    idle_head.prev->next = idle;
    idle_head.prev = idle;
}

void unregister_kernel_idle(struct kernel_idle *idle)
{
    idle->prev->next = idle->next;
    idle->next->prev = idle->prev;
    idle->prev = idle->next = NULL;
}

void kernel_main()
{
    for (;;)
    {
        struct kernel_idle *idle = idle_head.next;
        for (; idle != &idle_head; idle = idle->next)
            idle->idle_func(idle->data);
    }
}
