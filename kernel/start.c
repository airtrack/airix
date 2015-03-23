#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/pit.h>
#include <kernel/exception.h>
#include <kernel/klib.h>
#include <mm/pmm.h>
#include <mm/paging.h>

struct boot_info
{
    physical_addr_t kernel_end;
    uint32_t num_mmap_entries;
    struct mmap_entry mmap_entries[1];
};

static void isr_timer()
{
}

/* Init kernel */
void init_kernel(physical_addr_t bi)
{
    struct boot_info *binfo = (void *)bi;

    /* Init paging */
    VIRTUAL_TO_PHYSICAL(init_paging)(ALIGN_PAGE(binfo->kernel_end));

    /* Now all addresses are virtual address */
    clear_screen();
    printk("Init kernel ...\n");

    init_gdt();
    init_idt();
    init_pic();
    init_pit(50);
    init_exception_handle();

    pic_register_isr(IRQ0, isr_timer);
    printk("Success!\n");
}
