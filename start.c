#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "klib.h"

static void isr_timer()
{
    static char c = 0;
    display_char(20, 10, c++);
}

/* C entry */
void cstart(void *mm)
{
    init_gdt();
    init_idt();
    init_pic();
    init_pit(50);

    pic_register_isr(IRQ0, isr_timer);
    display_char(20, 5, *(int *)mm + '0');
}
