#include <kernel/pci.h>
#include <kernel/klib.h>
#include <kernel/ide.h>

#define PCI_ENABLE_FLAG 0x80000000
#define INVALID_VENDOR_ID 0xffff

static uint32_t config_read(uint32_t bus, uint32_t device,
                            uint32_t func, uint32_t offset)
{
    uint32_t address = PCI_ENABLE_FLAG | (bus << 16) |
        (device << 11) | (func << 8) | (offset & 0xfc);

    out_dword(PCI_CONFIG_ADDRESS_PORT, address);
    return in_dword(PCI_CONFIG_DATA_PORT);
}

static inline uint16_t config_read_low_word(uint32_t bus, uint32_t device,
                                            uint32_t func, uint32_t offset)
{
    return config_read(bus, device, func, offset) & 0xffff;
}

static inline uint16_t config_read_high_word(uint32_t bus, uint32_t device,
                                             uint32_t func, uint32_t offset)
{
    return (config_read(bus, device, func, offset) >> 16) & 0xffff;
}

static void check_device_function(uint8_t bus, uint8_t device, uint8_t func)
{
    uint32_t code = config_read(bus, device, func, 0x8);
    uint8_t class = (code >> 24) & 0xff;
    uint8_t subclass = (code >> 16) & 0xff;
    uint8_t prog_if = (code >> 8) & 0xff;

    if (class == PCI_CLASS_MASS_STORAGE && subclass == PCI_SUBCLASS_IDE &&
        (prog_if == 0x8a || prog_if == 0x80))
    {
        ide_initialize();
    }
}

static void check_device(uint8_t bus, uint8_t device)
{
    uint8_t func = 0;
    uint16_t vendor_id = config_read_low_word(bus, device, func, 0);

    if (vendor_id == INVALID_VENDOR_ID)
        return ;

    check_device_function(bus, device, func);

    /* Check head type if it is multi-function device. */
    if (config_read_high_word(bus, device, func, 0xc) & 0x80)
    {
        for (func = 1; func < 8; ++func)
        {
            vendor_id = config_read_low_word(bus, device, func, 0);
            if (vendor_id != INVALID_VENDOR_ID)
                check_device_function(bus, device, func);
        }
    }
}

void pci_detecting_devices()
{
    for (uint32_t bus = 0; bus < 256; ++bus)
        for (uint32_t device = 0; device < 32; ++device)
            check_device(bus, device);
}
