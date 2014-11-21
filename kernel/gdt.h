#ifndef GDT_H
#define GDT_H

#include <kernel/base_types.h>

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
typedef struct
{
    uint16_t limit;
    gdt_entry_t *base;
} gdtr_t;

#pragma pack(pop)

/* Number of GDT entries */
#define GDT_ENTRY_NUM 100

/* GDT selectors */
#define GDT_FLAT_MEM_TEXT_SEL 0x8
#define GDT_FLAT_MEM_DATA_SEL 0x10

/* DPL values */
#define DPL_0 0
#define DPL_1 1
#define DPL_2 2
#define DPL_3 3

/* GDT init function */
void init_gdt();

#endif // GDT_H
