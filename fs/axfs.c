#include "fs/axfs.h"
#include "fs/fs.h"

static int ax_open(struct file *file)
{
    (void)file;
    return 0;
}

static int ax_read(struct file *file, char *buffer, size_t size)
{
    (void)file, (void)buffer, (void)size;
    return 0;
}

static int ax_write(struct file *file, const char *buffer, size_t size)
{
    (void)file, (void)buffer, (void)size;
    return 0;
}

static const struct file_operations ops =
{
    .open       = ax_open,
    .read       = ax_read,
    .write      = ax_write
};

const struct file_system axfs =
{
    .name       = "axfs",
    .op         = &ops
};
