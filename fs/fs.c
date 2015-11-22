#include <fs/fs.h>
#include <mm/slab.h>
#include <kernel/klib.h>
#include <string.h>

#define VFS_ROOT_DIR '/'

static struct mount root;
static struct kmem_cache *file_cache;
static struct kmem_cache *inode_cache;

extern const struct file_system axfs;

static const struct mount * find_mount(const char *path)
{
    if (*path != VFS_ROOT_DIR)
        return NULL;
    return &root;
}

static inline char *path_dup(const char *path)
{
    char *dup = kmalloc(strlen(path) + 1);
    if (!dup) return NULL;
    return strcpy(dup, path);
}

static inline void free_path(char **path)
{
    kfree(*path);
    *path = NULL;
}

static inline struct inode * alloc_inode()
{
    struct inode *inode = slab_alloc(inode_cache);
    if (inode)
        memset(inode, 0, sizeof(*inode));
    return inode;
}

static inline void free_inode(struct inode **inode)
{
    slab_free(inode_cache, *inode);
    *inode = NULL;
}

void vfs_initialize()
{
    root.device = 0;
    root.mount_root = "/";
    root.fs = &axfs;

    file_cache = slab_create_kmem_cache(sizeof(struct file), sizeof(void *));
    inode_cache = slab_create_kmem_cache(sizeof(struct inode), sizeof(void *));

    /* Initialize file systems */
    axfs.initialize();
}

struct file * vfs_alloc_file()
{
    struct file *file = slab_alloc(file_cache);
    if (file)
        memset(file, 0, sizeof(*file));
    return file;
}

void vfs_free_file(struct file *file)
{
    slab_free(file_cache, file);
}

int vfs_open(struct file *file, const char *path, int flags)
{
    int error;

    file->f_mount = find_mount(path);
    if (!file->f_mount)
        panic("Can not find file '%s' mount path.", path);

    file->f_path = path_dup(path);
    file->f_inode = alloc_inode();
    file->f_op = file->f_mount->fs->op;
    file->f_flags = flags;

    if (!file->f_path || !file->f_inode)
        panic("Alloc file struct data failed.");

    file->f_inode->i_device = file->f_mount->device;

    error = file->f_op->open(file);
    if (error != 0)
    {
        free_path(&file->f_path);
        free_inode(&file->f_inode);
    }

    return error;
}

int vfs_read(struct file *file, char *buffer, size_t bytes)
{
    return file->f_op->read(file, buffer, bytes);
}

int vfs_write(struct file *file, const char *data, size_t bytes)
{
    return file->f_op->write(file, data, bytes);
}

int vfs_close(struct file *file)
{
    file->f_op->close(file);
    free_path(&file->f_path);
    free_inode(&file->f_inode);
    return 0;
}
