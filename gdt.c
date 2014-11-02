#include "gdt.h"

gdt_entry gdt[GDT_ENTRY_NUM];
gdtr_t gdtr = { sizeof(gdt) - 1, gdt };
