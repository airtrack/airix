#ifndef GDT_H
#define GDT_H

#include "basetypes.h"

#pragma pack(push, 1)

/* GDT entry */
typedef struct
{
    uint16_t limit0;
    uint16_t base01;
    uint8_t base2;
    uint8_t access;
    uint8_t limit1:4;
    uint8_t flags:4;
    uint8_t base3;
} gdt_entry;

/* GDTR register data */
typedef struct
{
    uint16_t limit;
    gdt_entry *base;
} gdtr_t;

#pragma pack(pop)

/* Number of GDT entries */
#define GDT_ENTRY_NUM 100

/* GDTR */
extern gdtr_t gdtr;

#endif // GDT_H
