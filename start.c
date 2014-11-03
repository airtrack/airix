#include "gdt.h"
#include "idt.h"
#include "klib.h"

/* C entry */
void cstart()
{
    init_gdt();
    init_idt();

    for (unsigned char c = 0;; ++c)
        display_char(20, 10, c);
}
