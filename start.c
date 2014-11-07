#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "klib.h"

void isr_timer()
{
    static char c = 0;
    display_char(20, 10, c++);
}

/* C entry */
void cstart()
{
    init_gdt();
    init_idt();
    init_pic();

    pic_register_isr(IRQ0, isr_timer);
}
