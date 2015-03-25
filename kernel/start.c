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
    physical_addr_t free_addr;
    uint32_t num_mmap_entries;
    struct mmap_entry mmap_entries[1];
};

static void isr_timer()
{
}

static struct boot_info *boot_info;

void init_paging(physical_addr_t bi)
{
    struct boot_info *binfo = (void *)bi;
    physical_addr_t free = ALIGN_PAGE(binfo->free_addr);

    /* Init paging for kernel */
    free = VIRTUAL_TO_PHYSICAL(pg_init_paging)(free);
    /* Now all addresses are virtual address */

    /* Cast to virtual address */
    boot_info = CAST_PHYSICAL_TO_VIRTUAL(bi);
    boot_info->free_addr = free;
}

void kernel_main();

/* Kernel entry */
void kernel_entry()
{
    physical_addr_t free = boot_info->free_addr;

    /* Complete setup paging for kernel */
    free = pg_complete_paging(free, boot_info->mmap_entries,
                              boot_info->num_mmap_entries);

    clear_screen();
    printk("Init kernel ...\n");

    gdt_initialize();
    idt_initialize();
    pic_initialize();
    pit_initialize(50);
    exception_handle_initialize();

    pic_register_isr(IRQ0, isr_timer);
    printk("Success!\n");

    start_int();
    kernel_main();
}
