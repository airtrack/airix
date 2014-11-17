#ifndef IDT_H
#define IDT_H

#include <kernal/base_types.h>

#pragma pack(push, 1)

/* IDT entry */
typedef struct
{
    uint16_t offset_0;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_1;
} idt_entry_t;

/* IDTR register data */
typedef struct
{
    uint16_t limit;
    idt_entry_t *base;
} idtr_t;

#pragma pack(pop)

/* Number of IDT entries */
#define IDT_ENTRY_NUM 256

/* IDT types */
#define IDT_TYPE_TASK 0x5
#define IDT_TYPE_INT 0xe
#define IDT_TYPE_TRAP 0xf

/* IDT attrs */
#define IDT_ATTR_PRESENT 0x80

/* IDT init function */
void init_idt();

/* Set IDT entry */
void idt_set_entry(uint8_t entry_num, uint16_t selector,
        void *offset, uint8_t type, uint8_t dpl);

#endif // IDT_H
