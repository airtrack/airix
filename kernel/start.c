#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/pit.h>
#include <kernel/pci.h>
#include <kernel/ide.h>
#include <kernel/exception.h>
#include <kernel/keyboard.h>
#include <kernel/console.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/klib.h>
#include <mm/pmm.h>
#include <mm/paging.h>
#include <string.h>

struct boot_info
{
    physical_addr_t free_addr;
    uint32_t num_mmap_entries;
    struct mmap_entry mmap_entries[1];
};

static struct boot_info *boot_info;

static void test_console_char_consumer(console_char_t c, void *data)
{
    (void)data;
    if (c >= SHIFT('a') && c <= SHIFT('z'))
        put_char_at(17, 0, (c & 0xFF) - 32);
    else if (c >= SHIFT('A') && c <= SHIFT('Z'))
        put_char_at(17, 0, (c & 0xFF) + 32);
    else
        put_char_at(17, 0, c & 0xFF);
}

static void test_install_keyboard()
{
    static struct console console;
    struct key_code_handler handler;

    console.char_consumer = test_console_char_consumer;
    console.data = NULL;

    handler.handler = console_key_code_handler;
    handler.data = &console;
    kbd_set_key_code_handler(&handler);
}

static void test_exec()
{
    struct ide_io io;
    io.drive = 0;
    io.start = 0;
    io.sector_count = PAGE_SIZE / 512;
    io.buffer = pmm_alloc_page_address();
    io.size = PAGE_SIZE;

    if (!ide_read_sectors(&io))
    {
        printk("Read sectors error!\n");
        return ;
    }

    if (!proc_exec(CAST_PHYSICAL_TO_VIRTUAL(io.buffer), io.size))
    {
        printk("Execute process fail!\n");
    }
}

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
    printk("[%-8s] initialize kernel ...\n", "Entry");

    pmm_initialize(free, boot_info->mmap_entries,
                   boot_info->num_mmap_entries);

    gdt_initialize();
    idt_initialize();
    pic_initialize();
    pit_initialize(50);
    kbd_initialize();
    excep_initialize();
    pci_initialize();
    proc_initialize();
    sched_initialize();

    pmm_print_statistics(boot_info->mmap_entries,
                         boot_info->num_mmap_entries);

    printk("[%-8s] success!\n\n", "Entry");

    test_install_keyboard();
    test_exec();
    sched();

    kernel_main();
}
