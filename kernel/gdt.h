#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* Number of GDT entries */
#define GDT_ENTRY_NUM 6

/* GDT selectors */
#define KERNEL_CODE_SELECTOR 0x8
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR 0x18
#define USER_DATA_SELECTOR 0x20
#define TSS_SELECTOR 0x28

/* DPL values */
enum dpl
{
    DPL_0 = 0,
    DPL_1 = 1,
    DPL_2 = 2,
    DPL_3 = 3,
};

/* GDT init function */
void gdt_initialize();

/* Install TSS into GDT */
void gdt_install_tss(uint32_t base, uint32_t limit);

#endif /* GDT_H */
