#include <kernel/ide.h>
#include <kernel/klib.h>

struct drive
{
    uint8_t bus;            /* 0(Primary) or 1(Secondary) */
    uint8_t drive;          /* 0(Master) or 1(Slave) */
    uint16_t base_reg;      /* Base of registers */
    uint16_t control_reg;   /* Control/alternate status register */
    uint8_t exist;          /* ATA drive exist */
    uint8_t lba48;          /* Support LBA48 */
    uint64_t sectors;       /* Total sectors */
    char model[41];         /* Model string */
};

static struct drive drives[IDE_ATA_BUS_COUNT * IDE_ATA_DRIVE_COUNT];

static void detect_drive(struct drive *d)
{
    uint16_t indentify[256];
    /* Select drive */
    out_byte(d->base_reg + IDE_REGISTER_DRIVE_SELECT, 0xa0 | (d->drive << 4));

    /* Delay 400ns */
    in_byte(d->control_reg);
    in_byte(d->control_reg);
    in_byte(d->control_reg);
    in_byte(d->control_reg);

    out_byte(d->base_reg + IDE_REGISTER_SECTOR_COUNT, 0);
    out_byte(d->base_reg + IDE_REGISTER_LBA_LO, 0);
    out_byte(d->base_reg + IDE_REGISTER_LBA_MID, 0);
    out_byte(d->base_reg + IDE_REGISTER_LBA_HI, 0);

    /* Send identify command */
    out_byte(d->base_reg + IDE_REGISTER_COMMAND_STATUS,
             IDE_ATA_COMMAND_IDENTIFY);

    /* No drive */
    if (in_byte(d->base_reg + IDE_REGISTER_COMMAND_STATUS) == 0)
        return ;

    /* Polling */
    for (;;)
    {
        uint8_t status = in_byte(d->base_reg + IDE_REGISTER_COMMAND_STATUS);
        if (status & IDE_ATA_STATUS_ERR) return ;
        if (!(status & IDE_ATA_STATUS_BSY) && (status & IDE_ATA_STATUS_DRQ))
            break;
    }

    d->exist = 1;
    insw(d->base_reg + IDE_REGISTER_DATA, ARRAY_SIZE(indentify), indentify);

    /* Get LBA and sectors */
    if (indentify[83] & 0x0400)
    {
        /* LBA48 */
        d->lba48 = 1;
        d->sectors = *(uint64_t *)(indentify + 100);
    }
    else
    {
        /* LBA28 */
        d->lba48 = 0;
        d->sectors = *(uint32_t *)(indentify + 60);
    }

    /* Get model string */
    for (uint32_t i = 0; i < sizeof(d->model) - 1; i += 2)
    {
        const char *buffer = (char *)(indentify + 27 + i / 2);
        d->model[i] = buffer[1];
        d->model[i + 1] = buffer[0];
    }
    d->model[sizeof(d->model) - 1] = 0;
}

void ide_initialize()
{
    uint32_t count = 0;

    printk("[IDE] - find IDE:\n");

    for (uint32_t bus = IDE_ATA_BUS_PRIMARY; bus < IDE_ATA_BUS_COUNT; ++bus)
    {
        bool primary = bus == IDE_ATA_BUS_PRIMARY;
        uint16_t base_reg = primary ? IDE_BAR0 : IDE_BAR2;
        uint16_t control_reg = primary ? IDE_BAR1 + 2 : IDE_BAR3 + 2;

        for (uint32_t drive = IDE_ATA_DRIVE_MASTER;
             drive < IDE_ATA_DRIVE_COUNT; ++drive)
        {
            struct drive *d = &drives[bus * IDE_ATA_DRIVE_COUNT + drive];
            d->bus = bus;
            d->drive = drive;
            d->base_reg = base_reg;
            d->control_reg = control_reg;
            detect_drive(d);

            if (d->exist)
            {
                uint32_t bytes = d->sectors * 512;
                printk(" ATA%u-%u - %u bytes, model: %s\n",
                       d->bus, d->drive, bytes, d->model);
                ++count;
            }
        }
    }

    printk("[IDE] - total drives %u.\n", count);
}
