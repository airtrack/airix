#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/pit.h>
#include <kernel/klib.h>
#include <mm/pmm.h>
#include <mm/paging.h>

struct boot_info
{
    void *kernel_end;
    uint32_t num_mmap_entries;
    struct mmap_entry mmap_entries[1];
};

static void isr_timer()
{
}

/* Init kernel */
void init_kernel(struct boot_info *bi)
{
    clear_screen();
    printk("Init kernel ...\n");

    init_gdt();
    init_idt();
    init_pic();
    init_pit(50);
    init_pmm(bi->kernel_end, bi->mmap_entries, bi->num_mmap_entries);
    init_paging();

    pic_register_isr(IRQ0, isr_timer);
    printk("Success!\n");
}
