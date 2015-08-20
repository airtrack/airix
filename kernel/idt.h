#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#pragma pack(push, 1)

/* IDT entry */
struct idt_entry
{
    uint16_t offset_0;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_1;
};

/* IDTR register data */
struct idtr
{
    uint16_t limit;
    struct idt_entry *base;
};

#pragma pack(pop)

/* Number of IDT entries */
#define IDT_ENTRY_NUM 256

/* IDT types */
enum idt_type
{
    IDT_TYPE_TASK = 0x5,
    IDT_TYPE_INT = 0xE,
    IDT_TYPE_TRAP = 0xF,
};

/* IDT attrs */
#define IDT_ATTR_PRESENT 0x80

/* IDT init function */
void idt_initialize();

/* Set IDT entry */
void idt_set_entry(uint8_t entry_num, uint16_t selector,
                   void *offset, uint8_t type, uint8_t dpl);

#endif /* IDT_H */
