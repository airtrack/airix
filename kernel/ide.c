#include <kernel/ide.h>
#include <kernel/klib.h>
#include <kernel/pic.h>
#include <mm/slab.h>

enum bus_master_cmd
{
    BUS_MASTER_CMD_WRITE = 0x0,
    BUS_MASTER_CMD_READ = 0x8,
};

struct drive
{
    uint8_t bus;            /* 0(Primary) or 1(Secondary) */
    uint8_t drive;          /* 0(Master) or 1(Slave) */
    uint16_t bm_reg;        /* Base address of bus master register */
    uint16_t base_reg;      /* Base address of registers */
    uint16_t control_reg;   /* Control/alternate status register */
    uint8_t exist;          /* ATA drive exist */
    uint8_t lba48;          /* Support LBA48 */
    uint64_t sectors;       /* Total sectors */
    char model[41];         /* Model string */
};

struct prd_entry
{
    uint32_t physical_addr;
    uint16_t size;
    uint16_t reserved;
};

struct dma_io_data
{
    size_t size;
    physical_addr_t buffer;
    ide_on_io_complete_t complete_func;

    uint8_t drive;
    uint8_t bm_cmd;
    uint8_t cmd;
    uint64_t start_sector;
    uint16_t sector_count;

    struct dma_io_data *prev;
    struct dma_io_data *next;
};

static struct drive drives[IDE_ATA_BUS_COUNT * IDE_ATA_DRIVE_COUNT];
static struct prd_entry prdt[IDE_ATA_BUS_COUNT];
static struct dma_io_data dma_io_data[IDE_ATA_BUS_COUNT];
static struct kernel_idle ide_idle;
static struct kmem_cache *io_data_cache;

static void initialize_io_data()
{
    /* Initialize io data lists. */
    for (uint32_t i = 0; i < IDE_ATA_BUS_COUNT; ++i)
    {
        struct dma_io_data *io_data = &dma_io_data[i];
        io_data->prev = io_data->next = io_data;
    }

    /* Create struct dma_io_data cache */
    io_data_cache = slab_create_kmem_cache(sizeof(struct dma_io_data), sizeof(void *));
}

static void start_io_operation(const struct dma_io_data *data)
{
    uint8_t cmd = data->cmd;
    uint8_t bm_cmd = data->bm_cmd;
    uint16_t count = data->sector_count;
    uint64_t start = data->start_sector;
    struct drive *d = &drives[data->drive];
    struct prd_entry *prdt_address = &prdt[d->bus];

    /* Prepare PRDT */
    prdt_address->physical_addr = data->buffer;
    prdt_address->size = data->size;
    prdt_address->reserved = 0x8000;    /* Set it as the last PRD entry */

    /* Stop DMA first, and then send command. */
    out_byte(d->bm_reg + IDE_BUS_MASTER_CMD, 0x0);
    out_dword(d->bm_reg + IDE_BUS_MASTER_PRDT, CAST_VIRTUAL_TO_PHYSICAL(prdt_address));
    out_byte(d->bm_reg + IDE_BUS_MASTER_CMD, bm_cmd | 0x1);

    if (d->lba48)
    {
        out_byte(d->base_reg + IDE_REGISTER_DRIVE_SELECT, 0x40 | (d->drive << 4));
        out_byte(d->base_reg + IDE_REGISTER_SECTOR_COUNT, (count >> 8) & 0xff);
        out_byte(d->base_reg + IDE_REGISTER_LBA_LO, (start >> 24) & 0xff);
        out_byte(d->base_reg + IDE_REGISTER_LBA_MID, (start >> 32) & 0xff);
        out_byte(d->base_reg + IDE_REGISTER_LBA_HI, (start >> 40) & 0xff);
        out_byte(d->base_reg + IDE_REGISTER_SECTOR_COUNT, count & 0xff);
        out_byte(d->base_reg + IDE_REGISTER_LBA_LO, start & 0xff);
        out_byte(d->base_reg + IDE_REGISTER_LBA_MID, (start >> 8) & 0xff);
        out_byte(d->base_reg + IDE_REGISTER_LBA_HI, (start >> 16) & 0xff);
    }
    else
    {
        out_byte(d->base_reg + IDE_REGISTER_DRIVE_SELECT,
                 0xe0 | (d->drive << 4) | ((start >> 24) & 0x0f));
        out_byte(d->base_reg + IDE_REGISTER_FEATURE_ERROR, 0);
        out_byte(d->base_reg + IDE_REGISTER_SECTOR_COUNT, count);
        out_byte(d->base_reg + IDE_REGISTER_LBA_LO, start & 0xff);
        out_byte(d->base_reg + IDE_REGISTER_LBA_MID, (start >> 8) & 0xff);
        out_byte(d->base_reg + IDE_REGISTER_LBA_HI, (start >> 16) & 0xff);
    }

    out_byte(d->base_reg + IDE_REGISTER_COMMAND_STATUS, cmd);
}

static void check_if_io_complete(uint8_t bus)
{
    struct dma_io_data *dma_data_head = &dma_io_data[bus];
    struct dma_io_data *dma_data = dma_data_head->next;

    if (dma_data != dma_data_head)
    {
        struct drive *d = &drives[dma_data->drive];
        uint8_t status = in_byte(d->bm_reg + IDE_BUS_MASTER_STATUS);

        if (status & 0x2)
        {
            /* TODO: error, reset the bus. */
            out_byte(d->bm_reg + IDE_BUS_MASTER_STATUS, 0x2);
        }
        else if (!(status & 0x1))
        {
            /* IO completed */
            size_t size = dma_data->size;
            physical_addr_t buffer = dma_data->buffer;
            ide_on_io_complete_t func = dma_data->complete_func;

            /* Remove from list and free the struct */
            dma_data->prev->next = dma_data->next;
            dma_data->next->prev = dma_data->prev;
            dma_data->prev = dma_data->next = NULL;
            slab_free(io_data_cache, dma_data);

            /* Start next IO operation */
            if (dma_data_head->next != dma_data_head)
                start_io_operation(dma_data_head->next);

            func(buffer, size);
        }
    }
}

static void idle_function(void *data)
{
    (void)data;

    for (uint32_t i = IDE_ATA_BUS_PRIMARY; i < IDE_ATA_BUS_COUNT; ++i)
        check_if_io_complete(i);
}

static void irq_isr14()
{
}

static void irq_isr15()
{
}

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

void ide_initialize(uint16_t bm_dma)
{
    uint32_t count = 0;
    bool primary_exist = false;
    bool secondary_exist = false;

    printk("[IDE] - find IDE:\n");

    for (uint32_t bus = IDE_ATA_BUS_PRIMARY; bus < IDE_ATA_BUS_COUNT; ++bus)
    {
        bool primary = bus == IDE_ATA_BUS_PRIMARY;
        uint16_t bm_reg = primary ? bm_dma : bm_dma + 0x8;
        uint16_t base_reg = primary ? IDE_BAR0 : IDE_BAR2;
        uint16_t control_reg = primary ? IDE_BAR1 + 2 : IDE_BAR3 + 2;

        for (uint32_t drive = IDE_ATA_DRIVE_MASTER; drive < IDE_ATA_DRIVE_COUNT; ++drive)
        {
            struct drive *d = &drives[bus * IDE_ATA_DRIVE_COUNT + drive];
            d->bus = bus;
            d->drive = drive;
            d->bm_reg = bm_reg;
            d->base_reg = base_reg;
            d->control_reg = control_reg;
            detect_drive(d);

            if (d->exist)
            {
                if (primary) primary_exist = true;
                else secondary_exist = true;

                uint32_t bytes = d->sectors * 512;
                printk("  [ATA%u-%u] - %u bytes, model: %s\n",
                       d->bus, d->drive, bytes, d->model);
                ++count;
            }
        }
    }

    initialize_io_data();

    /* Register idle function when there is one drive at least. */
    if (count > 0)
    {
        ide_idle.idle_func = idle_function;
        ide_idle.data = NULL;
        register_kernel_idle(&ide_idle);
    }

    if (primary_exist) pic_register_isr(IRQ14, irq_isr14);
    if (secondary_exist) pic_register_isr(IRQ15, irq_isr15);

    printk("[IDE] - total drives %u.\n", count);
}

static void dma_io_sectors(uint8_t drive, uint64_t start, uint16_t sector_count,
                           const struct ide_dma_io_data *data,
                           uint8_t bm_cmd, uint8_t cmd)
{
    struct dma_io_data *dma_data = slab_alloc(io_data_cache);
    struct dma_io_data *dma_data_head = &dma_io_data[drives[drive].bus];

    dma_data->size = data->size;
    dma_data->buffer = data->buffer;
    dma_data->complete_func = data->complete_func;
    dma_data->drive = drive;
    dma_data->bm_cmd = bm_cmd;
    dma_data->cmd = cmd;
    dma_data->start_sector = start;
    dma_data->sector_count = sector_count;

    if (dma_data_head->next == dma_data_head)
    {
        /* List is empty, start IO operation. */
        start_io_operation(dma_data);
    }

    /* Insert into io data list. */
    dma_data->prev = dma_data_head->prev;
    dma_data->next = dma_data_head;
    dma_data_head->prev->next = dma_data;
    dma_data_head->prev = dma_data;
}

static void check_drive(uint8_t drive, uint64_t start, uint16_t sector_count,
                        const struct ide_dma_io_data *io_data)
{
    struct drive *d = NULL;
    if (drive >= IDE_ATA_BUS_COUNT * IDE_ATA_DRIVE_COUNT || !drives[drive].exist)
        panic("[IDE] - drive not exist, drive no: %d.", drive);

    d = &drives[drive];
    if (start >= d->sectors || d->sectors - start < sector_count)
        panic("[IDE] - out of sector's range, total sectors: %u, start: %u, count: %u.",
              (uint32_t)d->sectors, (uint32_t)start, sector_count);

    if (io_data->size != sector_count * 512)
        panic("[IDE] - io buffer size(%u) != 512 * sector_count(%u).",
              io_data->size, sector_count);
}

void ide_dma_read_sectors(uint8_t drive, uint64_t start, uint16_t sector_count,
                          const struct ide_dma_io_data *io_data)
{
    uint8_t cmd = 0;
    check_drive(drive, start, sector_count, io_data);
    cmd = drives[drive].lba48 ? IDE_ATA_COMMAND_READ_DMA_EXT : IDE_ATA_COMMAND_READ_DMA;
    dma_io_sectors(drive, start, sector_count, io_data, BUS_MASTER_CMD_READ, cmd);
}

void ide_dma_write_sectors(uint8_t drive, uint64_t start, uint16_t sector_count,
                           const struct ide_dma_io_data *io_data)
{
    uint8_t cmd = 0;
    check_drive(drive, start, sector_count, io_data);
    cmd = drives[drive].lba48 ? IDE_ATA_COMMAND_WRITE_DMA_EXT : IDE_ATA_COMMAND_WRITE_DMA;
    dma_io_sectors(drive, start, sector_count, io_data, BUS_MASTER_CMD_WRITE, cmd);
}
