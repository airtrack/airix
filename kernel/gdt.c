#include <kernel/gdt.h>
#include <kernel/klib.h>

#pragma pack(push, 1)

/* GDT entry */
struct gdt_entry
{
    uint16_t limit_0;
    uint16_t base_0;
    uint8_t base_1;
    uint8_t access;
    uint8_t limit_1:4;
    uint8_t flags:4;
    uint8_t base_2;
};

/* GDTR register data */
struct gdtr
{
    uint16_t limit;
    struct gdt_entry *base;
};

#pragma pack(pop)

/* GDT */
static struct gdt_entry gdt[GDT_ENTRY_NUM];

/* GDTR */
static struct gdtr gdtr = { sizeof(gdt) - 1, gdt };

void gdt_initialize()
{
    gdt_set_descriptor(KERNEL_CODE_SELECTOR, 0, 0xFFFFF, 1, DPL_0);
    gdt_set_descriptor(KERNEL_DATA_SELECTOR, 0, 0xFFFFF, 0, DPL_0);
    gdt_set_descriptor(USER_CODE_SELECTOR, 0, 0xFFFFF, 1, DPL_3);
    gdt_set_descriptor(USER_DATA_SELECTOR, 0, 0xFFFFF, 0, DPL_3);
    set_gdtr(&gdtr);
}

void gdt_set_descriptor(uint16_t selector, uint32_t base,
                        uint32_t limit, uint8_t exec, uint8_t dpl)
{
    uint16_t index = selector / 8;
    struct gdt_entry *descriptor = &gdt[index];

    descriptor->base_0 = base & 0xFFFF;
    descriptor->base_1 = (base >> 16) & 0xFF;
    descriptor->base_2 = (base >> 24) & 0xFF;
    descriptor->limit_0 = limit & 0xFFFF;
    descriptor->limit_1 = (limit >> 16) & 0xF;
    descriptor->flags = 0xC;
    descriptor->access = 0x92 | ((dpl & 0x3) << 5) | ((exec & 0x1) << 3);
}
