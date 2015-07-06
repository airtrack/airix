#ifndef GDT_H
#define GDT_H

#include <kernel/base.h>

#pragma pack(push, 1)

/* GDT entry */
typedef struct
{
    uint16_t limit_0;
    uint16_t base_0;
    uint8_t base_1;
    uint8_t access;
    uint8_t limit_1:4;
    uint8_t flags:4;
    uint8_t base_2;
} gdt_entry_t;

/* GDTR register data */
struct gdtr
{
    uint16_t limit;
    gdt_entry_t *base;
};

#pragma pack(pop)

/* Number of GDT entries */
#define GDT_ENTRY_NUM 100

/* GDT selectors */
#define GDT_FLAT_MEM_TEXT_SEL 0x8
#define GDT_FLAT_MEM_DATA_SEL 0x10

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

#endif /* GDT_H */
