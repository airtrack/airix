#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/pit.h>
#include <kernel/pci.h>
#include <kernel/exception.h>
#include <kernel/keyboard.h>
#include <kernel/console.h>
#include <kernel/klib.h>
#include <mm/pmm.h>
#include <mm/paging.h>

struct boot_info
{
    physical_addr_t free_addr;
    uint32_t num_mmap_entries;
    struct mmap_entry mmap_entries[1];
};

static struct boot_info *boot_info;

static void test_isr_timer()
{
    static char c = 0;
    put_char_at(18, 0, c++);
}

static void test_console_char_consumer(console_char_t c, void *data)
{
    (void)data;
    if (c >= SHIFT('a') && c <= SHIFT('z'))
        put_char_at(15, 0, (c & 0xFF) - 32);
    else if (c >= SHIFT('A') && c <= SHIFT('Z'))
        put_char_at(15, 0, (c & 0xFF) + 32);
    else
        put_char_at(15, 0, c & 0xFF);
}

static void install_keyboard_test()
{
    static struct console console;
    struct key_code_handler handler;

    console.char_consumer = test_console_char_consumer;
    console.data = NULL;

    handler.handler = console_key_code_handler;
    handler.data = &console;
    kbd_set_key_code_handler(&handler);
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
    printk("Init kernel ...\n");

    pmm_initialize(free, boot_info->mmap_entries, boot_info->num_mmap_entries);

    gdt_initialize();
    idt_initialize();
    pic_initialize();
    pit_initialize(50);
    kbd_initialize();
    exception_handle_initialize();

    pci_detecting_devices();
    pic_register_isr(IRQ0, test_isr_timer);
    install_keyboard_test();

    pmm_print_statistics(boot_info->mmap_entries, boot_info->num_mmap_entries);
    printk("Success!\n");

    start_int();
    kernel_main();
}
