#include <kernel/scheduler.h>
#include <kernel/klib.h>
#include <kernel/gdt.h>

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

void sched_initialize()
{
    uint32_t base = (uint32_t)&tss;
    gdt_install_tss(base, base + sizeof(tss));
    set_tss(TSS_SELECTOR | DPL_3);
}

void sched_process(struct process *proc)
{
    /* Update TSS */
    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.esp0 = proc->kernel_stack;

    /* Change virtual address space */
    set_cr3(CAST_VIRTUAL_TO_PHYSICAL(proc->page_dir));

    /* TODO: Run process proc */
}
