#include <kernel/ide.h>
#include <kernel/ktask.h>
#include <kernel/klib.h>
#include <kernel/pic.h>
#include <mm/slab.h>

/* Base address register of IDE */
enum ide_bar
{
    IDE_BAR0 = 0x1f0,
    IDE_BAR1 = 0x3f4,
    IDE_BAR2 = 0x170,
    IDE_BAR3 = 0x374,
};

enum ide_ata_status
{
    IDE_ATA_STATUS_ERR = 0x01,
    IDE_ATA_STATUS_DRQ = 0x08,
    IDE_ATA_STATUS_SRV = 0x10,
    IDE_ATA_STATUS_DF = 0x20,
    IDE_ATA_STATUS_RDY = 0x40,
    IDE_ATA_STATUS_BSY = 0x80
};

enum ide_ata_command
{
    IDE_ATA_COMMAND_READ_PIO = 0x20,
    IDE_ATA_COMMAND_READ_PIO_EXT = 0x24,
    IDE_ATA_COMMAND_READ_DMA = 0xc8,
    IDE_ATA_COMMAND_READ_DMA_EXT = 0x25,
    IDE_ATA_COMMAND_WRITE_PIO = 0x30,
    IDE_ATA_COMMAND_WRITE_PIO_EXT = 0x34,
    IDE_ATA_COMMAND_WRITE_DMA = 0xca,
    IDE_ATA_COMMAND_WRITE_DMA_EXT = 0x35,
    IDE_ATA_COMMAND_CACHE_FLUSH = 0xe7,
    IDE_ATA_COMMAND_CACHE_FLUSH_EXT = 0xea,
    IDE_ATA_COMMAND_PACKET = 0xa0,
    IDE_ATA_COMMAND_IDENTIFY_PACKET = 0xa1,
    IDE_ATA_COMMAND_IDENTIFY = 0xec,
};

enum ide_ata_bus
{
    IDE_ATA_BUS_PRIMARY = 0,
    IDE_ATA_BUS_SECONDARY,
    IDE_ATA_BUS_COUNT,
};

enum ide_ata_drive
{
    IDE_ATA_DRIVE_MASTER = 0,
    IDE_ATA_DRIVE_SLAVE,
    IDE_ATA_DRIVE_COUNT,
};

enum ide_register
{
    IDE_REGISTER_DATA = 0,
    IDE_REGISTER_FEATURE_ERROR,
    IDE_REGISTER_SECTOR_COUNT,
    IDE_REGISTER_LBA_LO,
    IDE_REGISTER_LBA_MID,
    IDE_REGISTER_LBA_HI,
    IDE_REGISTER_DRIVE_SELECT,
    IDE_REGISTER_COMMAND_STATUS,
};

/* IO ports offset of bus master */
enum ide_bus_master
{
    IDE_BUS_MASTER_CMD = 0,
    IDE_BUS_MASTER_STATUS = 2,
    IDE_BUS_MASTER_PRDT = 4,
};

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
    struct ide_dma_io io;

    uint8_t retry;
    uint8_t bm_cmd;
    uint8_t cmd;

    struct dma_io_data *prev;
    struct dma_io_data *next;
};

static struct drive drives[IDE_ATA_BUS_COUNT * IDE_ATA_DRIVE_COUNT];
static struct prd_entry prdt[IDE_ATA_BUS_COUNT];
static struct dma_io_data dma_io_data[IDE_ATA_BUS_COUNT];
static struct kernel_task ide_task;
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
    uint16_t count = data->io.sector_count;
    uint64_t start = data->io.start;
    struct drive *d = &drives[data->io.drive];
    struct prd_entry *prdt_address = &prdt[d->bus];

    /* Prepare PRDT */
    prdt_address->physical_addr = data->io.buffer;
    prdt_address->size = data->io.size;
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

static void complete_io_operation(struct dma_io_data *dma_data_head,
                                  struct dma_io_data *dma_data, bool error)
{
    struct ide_dma_io io = dma_data->io;

    /* Remove from list and free the struct */
    dma_data->prev->next = dma_data->next;
    dma_data->next->prev = dma_data->prev;
    dma_data->prev = dma_data->next = NULL;
    slab_free(io_data_cache, dma_data);

    /* Start next IO operation */
    if (dma_data_head->next != dma_data_head)
        start_io_operation(dma_data_head->next);

    io.complete_func(&io, error);
}

static void check_if_io_complete(uint8_t bus)
{
    struct dma_io_data *dma_data_head = &dma_io_data[bus];
    struct dma_io_data *dma_data = dma_data_head->next;

    if (dma_data != dma_data_head)
    {
        struct drive *d = &drives[dma_data->io.drive];
        uint8_t status = in_byte(d->bm_reg + IDE_BUS_MASTER_STATUS);

        if (status & 0x2)
        {
            /* Error, clear bus master status bit 1 */
            out_byte(d->bm_reg + IDE_BUS_MASTER_STATUS, 0x2);

            /* Reset drives */
            out_byte(d->control_reg, 0x4);
            out_byte(d->control_reg, 0);

            /* Retry if the error is the first occurrence */
            if (dma_data->retry == 0)
            {
                dma_data->retry = 1;
                start_io_operation(dma_data);
            }
            else
            {
                complete_io_operation(dma_data_head, dma_data, true);
            }
        }
        else if (!(status & 0x1))
        {
            complete_io_operation(dma_data_head, dma_data, false);
        }
    }
}

static void task_function(void *data)
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

    printk("[%-8s] find IDE:\n", "IDE");

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

                uint32_t bytes = d->sectors * SECTOR_SIZE;
                printk("  [ATA%u-%u] %u bytes, model: %s\n",
                       d->bus, d->drive, bytes, d->model);
                ++count;
            }
        }
    }

    initialize_io_data();

    /* Register task function when there is one drive at least. */
    if (count > 0)
    {
        ide_task.task_func = task_function;
        ide_task.data = NULL;
        ktask_register(&ide_task);
    }

    if (primary_exist) pic_register_isr(IRQ14, irq_isr14);
    if (secondary_exist) pic_register_isr(IRQ15, irq_isr15);

    printk("[%-8s] total drives %u.\n", "IDE", count);
}

static void dma_io_sectors(const struct ide_dma_io *io,
                           uint8_t bm_cmd, uint8_t cmd)
{
    struct dma_io_data *dma_data = slab_alloc(io_data_cache);
    struct dma_io_data *dma_data_head = &dma_io_data[drives[io->drive].bus];

    dma_data->io = *io;
    dma_data->retry = 0;
    dma_data->bm_cmd = bm_cmd;
    dma_data->cmd = cmd;

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

static void check_drive(const struct ide_dma_io *io_data)
{
    struct drive *d = NULL;
    uint8_t drive = io_data->drive;
    uint64_t start = io_data->start;
    uint16_t sector_count = io_data->sector_count;

    if (drive >= IDE_ATA_BUS_COUNT * IDE_ATA_DRIVE_COUNT || !drives[drive].exist)
        panic("[IDE] - drive not exist, drive no: %d.", drive);

    d = &drives[drive];
    if (start >= d->sectors || d->sectors - start < sector_count)
        panic("[IDE] - out of sector's range, total sectors: %u, start: %u, count: %u.",
              (uint32_t)d->sectors, (uint32_t)start, sector_count);

    if (io_data->size != sector_count * SECTOR_SIZE)
        panic("[IDE] - io buffer size(%u) != 512 * sector_count(%u).",
              io_data->size, sector_count);
}

void ide_dma_read_sectors(const struct ide_dma_io *io_data)
{
    uint8_t cmd = 0;
    check_drive(io_data);
    cmd = drives[io_data->drive].lba48 ?
        IDE_ATA_COMMAND_READ_DMA_EXT : IDE_ATA_COMMAND_READ_DMA;
    dma_io_sectors(io_data, BUS_MASTER_CMD_READ, cmd);
}

void ide_dma_write_sectors(const struct ide_dma_io *io_data)
{
    uint8_t cmd = 0;
    check_drive(io_data);
    cmd = drives[io_data->drive].lba48 ?
        IDE_ATA_COMMAND_WRITE_DMA_EXT : IDE_ATA_COMMAND_WRITE_DMA;
    dma_io_sectors(io_data, BUS_MASTER_CMD_WRITE, cmd);
}

static void sync_io_complete(struct ide_dma_io *io_data, bool error)
{
    *(bool *)io_data->data = error;
}

static inline bool sync_io_sectors(const struct ide_io *io_data,
                                   void (*io_func)(const struct ide_dma_io *))
{
    bool error = false;
    struct ide_dma_io io;

    io.drive = io_data->drive;
    io.start = io_data->start;
    io.sector_count = io_data->sector_count;
    io.buffer = io_data->buffer;
    io.size = io_data->size;
    io.data = &error;
    io.complete_func = sync_io_complete;

    io_func(&io);

    /* Wait io complete */
    for (uint8_t bus = drives[io.drive].bus;
         dma_io_data[bus].next != &dma_io_data[bus];)
        check_if_io_complete(bus);

    return !error;
}

bool ide_read_sectors(const struct ide_io *io_data)
{
    return sync_io_sectors(io_data, ide_dma_read_sectors);
}

bool ide_write_sectors(const struct ide_io *io_data)
{
    return sync_io_sectors(io_data, ide_dma_write_sectors);
}
