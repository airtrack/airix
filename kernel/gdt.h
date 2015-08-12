#ifndef GDT_H
#define GDT_H

#include <kernel/base.h>

/* Number of GDT entries */
#define GDT_ENTRY_NUM 6

/* GDT selectors */
#define KERNEL_CODE_SELECTOR 0x8
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR 0x18
#define USER_DATA_SELECTOR 0x20

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

/* Set GDT descriptor */
void gdt_set_descriptor(uint16_t selector, uint32_t base,
                        uint32_t limit, uint8_t exec, uint8_t dpl);

#endif /* GDT_H */
