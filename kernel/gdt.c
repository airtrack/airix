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

static inline struct gdt_entry * get_descriptor(uint16_t selector)
{
    uint16_t index = selector / sizeof(struct gdt_entry);
    return &gdt[index];
}

static void set_base_limit(struct gdt_entry *descriptor,
                           uint32_t base, uint32_t limit)
{
    descriptor->base_0 = base & 0xFFFF;
    descriptor->base_1 = (base >> 16) & 0xFF;
    descriptor->base_2 = (base >> 24) & 0xFF;
    descriptor->limit_0 = limit & 0xFFFF;
    descriptor->limit_1 = (limit >> 16) & 0xF;
}

static void set_code_data(uint16_t selector, uint32_t base,
                          uint32_t limit, uint8_t code, uint8_t dpl)
{
    struct gdt_entry *descriptor = get_descriptor(selector);
    set_base_limit(descriptor, base, limit);

    descriptor->flags = 0xC;
    descriptor->access = 0x92 | ((dpl & 0x3) << 5) | ((code & 0x1) << 3);
}

void gdt_initialize()
{
    set_code_data(KERNEL_CODE_SELECTOR, 0, 0xFFFFF, 1, DPL_0);
    set_code_data(KERNEL_DATA_SELECTOR, 0, 0xFFFFF, 0, DPL_0);
    set_code_data(USER_CODE_SELECTOR, 0, 0xFFFFF, 1, DPL_3);
    set_code_data(USER_DATA_SELECTOR, 0, 0xFFFFF, 0, DPL_3);
    set_gdtr(&gdtr);
}

void gdt_install_tss(uint32_t base, uint32_t limit)
{
    struct gdt_entry *descriptor = get_descriptor(TSS_SELECTOR);
    set_base_limit(descriptor, base, limit - 1);

    descriptor->flags = 0;
    descriptor->access = 0x89 | (DPL_0 << 5);
}
