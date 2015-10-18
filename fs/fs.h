#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

struct inode;
struct file_operations;
struct mount;

struct file
{
    const struct mount *f_mount;
    const char *f_path;
    struct inode *f_inode;
    const struct file_operations *f_op;

    int f_flags;
};

struct file_operations
{
    int (*open)(struct file *);
    int (*read)(struct file *, char *, size_t);
    int (*write)(struct file *, const char *, size_t);
};

struct inode
{
    uint8_t dev;
};

struct file_system
{
    const char *name;                   /* File system name */
    const struct file_operations *op;   /* File operations of file system */
};

struct mount
{
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
