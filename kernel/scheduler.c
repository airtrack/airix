#include <kernel/scheduler.h>
#include <kernel/klib.h>
#include <kernel/gdt.h>
#include <kernel/pic.h>

struct tss
{
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldtr;
    uint16_t trap;
    uint16_t iomap;
};

static struct tss tss;

static struct process list_head;
static struct process *current_proc;

static inline void flush_tss()
{
    uint32_t base = (uint32_t)&tss;
    gdt_install_tss(base, base + sizeof(tss));
    set_tss(TSS_SELECTOR);
}

static void sched_timer()
{
    /*
     * Send EOI first, because sched
     * will run into the user space of a process
     */
    pic_send_eoi(IRQ0);
    sched();
}

void sched_initialize()
{
    list_head.prev = &list_head;
    list_head.next = &list_head;

    flush_tss();
    pic_register_isr(IRQ0, sched_timer);
}

static void init_trap_frame(struct process *proc)
{
    char *stack = (char *)proc->kernel_stack;
    proc->trap = (struct trap_frame *)(stack - sizeof(*proc->trap));

    /* Initialize segment reigsters */
    proc->trap->gs = USER_DATA_SELECTOR | DPL_3;
    proc->trap->fs = USER_DATA_SELECTOR | DPL_3;
    proc->trap->es = USER_DATA_SELECTOR | DPL_3;
    proc->trap->ds = USER_DATA_SELECTOR | DPL_3;

    proc->trap->ss = USER_DATA_SELECTOR | DPL_3;
    proc->trap->cs = USER_CODE_SELECTOR | DPL_3;

    /* Initialize user stack, entry point and interrupt flag */
    proc->trap->user_esp = proc->user_stack;
    proc->trap->eflags = FLAGS_IF;
    proc->trap->eip = proc->entry;
    proc->trap->error_code = 0;
}

void sched_add(struct process *proc)
{
    proc->next = list_head.next;
    proc->prev = list_head.next->prev;
    list_head.next->prev = proc;
    list_head.next = proc;
}

void sched_process(struct process *proc)
{
    /* Close interrupt, interrupt will be enabled by iret */
    close_int();

    /* Update TSS */
    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.esp0 = proc->kernel_stack;
    flush_tss();

    /* Change virtual address space */
    set_cr3(CAST_VIRTUAL_TO_PHYSICAL(proc->page_dir));

    /* Prepare trap frame */
    if (!proc->trap)
        init_trap_frame(proc);

    /* Returns to user space, run user process */
    current_proc = proc;
    ret_user_space(proc->trap);
}

void sched()
{
    struct process *proc =
        current_proc ? current_proc->next : list_head.next;

    /* Find a runnable process */
    while (proc == &list_head || proc->state != PROC_STATE_RUNNING)
        proc = proc->next;

    sched_process(proc);
}
