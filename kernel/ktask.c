#include <kernel/ktask.h>
#include <kernel/klib.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <mm/paging.h>

static struct kernel_task task_head = { NULL, NULL, &task_head, &task_head };

static void ktask_main()
{
    for (;;)
    {
        struct kernel_task *task = task_head.next;
        for (; task != &task_head; task = task->next)
            task->task_func(task->data);

        close_int();
        sched();
        start_int();
    }
}

void ktask_initialize()
{
    /* Initialize kernel task process which is a ring0 process */
    struct process *ktask = proc_alloc();
    if (!ktask)
        panic("Kernel task process alloc fail");

    if (ktask->pid != 0)
        panic("Kernel task pid(%d) != 0", ktask->pid);

    ktask->kernel_stack = (uint32_t)cast_p2v_or_null(pmm_alloc_page_address());
    if ((void *)ktask->kernel_stack == NULL)
        panic("Kernel task stack alloc fail");

    ktask->kernel_stack += PAGE_SIZE;

    ktask->page_dir = vmm_alloc_vaddr_space();
    if (!ktask->page_dir)
        panic("Kernel task vmm alloc fail");

    ktask->state = PROC_STATE_RUNNING;
    ktask->entry = (uint32_t)ktask_main;

    pg_copy_kernel_space(ktask->page_dir);
    sched_add(ktask);
}

void ktask_register(struct kernel_task *task)
{
    task->prev = task_head.prev;
    task->next = &task_head;
    task_head.prev->next = task;
    task_head.prev = task;
}

void ktask_unregister(struct kernel_task *task)
{
    task->prev->next = task->next;
    task->next->prev = task->prev;
    task->prev = task->next = NULL;
}
