#ifndef IDE_H
#define IDE_H

#include <kernel/base.h>

struct ide_dma_io;
/*
 * Prototype of io complete function:
 *     void io_complete(struct ide_dma_io *io_data, bool error);
 * If error is true, then an error was occurred when doing IO.
 */
typedef void (*ide_on_io_complete_t)(struct ide_dma_io *, bool);

/* IO data struct for DMA read/write */
struct ide_dma_io
{
    uint8_t drive;
    uint16_t sector_count;
    uint64_t start;

    physical_addr_t buffer;
    size_t size;
    void *data;
    ide_on_io_complete_t complete_func;
};

void ide_initialize(uint16_t bm_dma);
void ide_dma_read_sectors(const struct ide_dma_io *io_data);
void ide_dma_write_sectors(const struct ide_dma_io *io_data);

#endif /* IDE_H */
