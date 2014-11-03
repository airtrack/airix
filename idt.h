#ifndef IDT_H
#define IDT_H

#include "base_types.h"

#pragma pack(push, 1)

/* IDT entry */
typedef struct
{
    uint16_t offset_0;
    uint16_t selector;
    uint8_t zero;
    uint8_t type:4;
    uint8_t attr:4;
    uint16_t offset_1;
} idt_entry_t;

/* IDTR register data */
typedef struct
{
    uint16_t limit;
    idt_entry_t *base;
} idtr_t;

#pragma pack(pop)

/* IDT types */
#define IDT_TYPE_TASK 0x5
#define IDT_TYPE_INT 0xe
#define IDT_TYPE_TRAP 0xf

/* IDT init function */
void init_idt();

#endif // IDT_H
