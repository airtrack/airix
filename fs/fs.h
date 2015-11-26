#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

enum open_flags
{
    O_RDONLY    = 1,
    O_WRONLY    = 1 << 1,
    O_RDWR      = 1 << 2,
    O_CREAT     = 1 << 3,
    O_TRUNC     = 1 << 4,
};

struct inode;
struct file_operations;
struct mount;

struct file
{
    const struct mount *f_mount;
    const struct file_operations *f_op;

    char *f_path;
    struct inode *f_inode;

    uint64_t f_pos;
    int f_refs;
    int f_flags;
};

struct file_operations
{
    int (*open)(struct file *);
    int (*close)(struct file *);
    int (*read)(struct file *, char *, size_t);
    int (*write)(struct file *, const char *, size_t);
};

struct inode
{
    uint8_t i_device;
    uint32_t i_ino;
    void *i_private;
};

struct file_system
{
    const char *name;                   /* File system name */
    const struct file_operations *op;   /* File operations of file system */
    void (*initialize)();               /* File system initialize function */
};

struct mount
{
    uint8_t device;                     /* Mount device ID */
    const char *mount_root;             /* File system mount root path */
    const struct file_system *fs;       /* File system type */
};

void vfs_initialize();
struct file * vfs_alloc_file();
void vfs_free_file(struct file *file);

int vfs_open(struct file *file, const char *path, int flags);
int vfs_read(struct file *file, char *buffer, size_t bytes);
int vfs_write(struct file *file, const char *data, size_t bytes);
int vfs_close(struct file *file);

#endif /* FS_H */
