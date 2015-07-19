#ifndef IDE_H
#define IDE_H

#include <kernel/base.h>

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

/*
 * Function will be called on DMA read/write completed.
 *   Prototype: void on_complete(physical_addr_t buffer,
 *                               uint32_t buffer_size,
 *                               bool error);
 * Parameter error is set true to indicate an error is occur when
 * doing read/write operation.
 */
typedef void (*ide_on_io_complete_t)(physical_addr_t, uint32_t, bool);

/* IO data struct for DMA read/write */
struct ide_dma_io_data
{
    size_t size;
    physical_addr_t buffer;
    ide_on_io_complete_t complete_func;
};

void ide_initialize(uint16_t bm_dma);

void ide_dma_read_sectors(uint8_t drive, uint64_t start, uint16_t sector_count,
                          const struct ide_dma_io_data *io_data);
void ide_dma_write_sectors(uint8_t drive, uint64_t start, uint16_t sector_count,
                           const struct ide_dma_io_data *io_data);

#endif /* IDE_H */
