#include <fs/axfs.h>
#include <fs/fs.h>
#include <mm/slab.h>
#include <kernel/bio.h>
#include <kernel/klib.h>
#include <stdbool.h>
#include <string.h>

#define SECTOR_NO(block) ((block) * AX_FS_SECTORS_PER_BLOCK)

static struct kmem_cache *inode_cache;

static void ax_fs_initialize()
{
    inode_cache = slab_create_kmem_cache(
        sizeof(struct ax_inode), sizeof(void *));
}

static void get_super_block(struct ax_super_block *sb, uint8_t device)
{
    uint64_t sb_sector = SECTOR_NO(AX_FS_SUPER_BLOCK_NO);
    struct bio *bio = bio_get(device, sb_sector);

    bio_read(bio);
    memcpy(sb, bio_data(bio), sizeof(*sb));
    bio_release(bio);
}

static inline uint32_t block_group(const struct ax_super_block *sb)
{
    return sb->s_blocks_count / AX_FS_BLOCKS_PER_GROUP +
        (sb->s_blocks_count % AX_FS_BLOCKS_PER_GROUP ? 1 : 0);
}

static inline uint32_t inode_bg_index(const struct ax_super_block *sb,
                                      uint32_t ino)
{
    return (ino - 1) / sb->s_inodes_per_group;
}

static inline uint32_t inode_index_in_bg(const struct ax_super_block *sb,
                                         uint32_t ino)
{
    return (ino - 1) % sb->s_inodes_per_group;
}

static inline uint32_t get_bg_inode_table(uint8_t device, uint32_t bg_index)
{
    uint32_t bg_inode_table = 0;

    uint32_t offset = sizeof(struct ax_block_group_descriptor) * bg_index;
    uint32_t sector = (offset % AX_FS_BLOCK_SIZE) / SECTOR_SIZE;
    uint64_t block = offset / AX_FS_BLOCK_SIZE + AX_FS_SUPER_BLOCK_NO;

    struct bio *bio = bio_get(device, SECTOR_NO(block) + sector);
    struct ax_block_group_descriptor *desc = NULL;

    bio_read(bio);
    desc = (struct ax_block_group_descriptor *)
        (bio_data(bio) + (offset % SECTOR_SIZE));
    bg_inode_table = desc->bg_inode_table;
    bio_release(bio);

    return bg_inode_table;
}

static void inode_read(struct ax_inode *inode, uint8_t device,
                       uint32_t bg_index, uint32_t index)
{
    uint32_t inode_table = get_bg_inode_table(device, bg_index);
    uint32_t offset = sizeof(struct ax_inode) * index;

    uint32_t sector = (offset % AX_FS_BLOCK_SIZE) / SECTOR_SIZE;
    uint32_t offset_in_sector = (offset % AX_FS_BLOCK_SIZE) % SECTOR_SIZE;
    uint64_t block = inode_table + offset / AX_FS_BLOCK_SIZE;

    struct bio *bio = bio_get(device, SECTOR_NO(block) + sector);
    bio_read(bio);
    memcpy(inode, bio_data(bio) + offset_in_sector, sizeof(*inode));
    bio_release(bio);
}

static struct ax_inode * inode_get(const struct ax_super_block *sb,
                                   uint8_t device, uint32_t ino)
{
    struct ax_inode *inode = slab_alloc(inode_cache);
    uint32_t bg_index = inode_bg_index(sb, ino);
    uint32_t index = inode_index_in_bg(sb, ino);
    if (inode) inode_read(inode, device, bg_index, index);
    return inode;
}

static inline void inode_release(struct ax_inode *inode)
{
    if (inode)
        slab_free(inode_cache, inode);
}

static uint32_t namex(const struct ax_super_block *sb,
                      const struct ax_inode *dir,
                      const char *name, uint8_t device)
{
    (void)sb, (void)dir,(void)name, (void)device;
    return 0;
}

static struct ax_inode * namei(const struct ax_super_block *sb,
                               const char *name, uint8_t device,
                               uint32_t *ino)
{
    struct ax_inode *inode = NULL;
    struct ax_inode *root = inode_get(sb, device, AX_FS_ROOT_INO);
    if (!root || root->i_mode != AX_S_IFDIR)
        panic("File system axfs: root inode does not exist.");

    *ino = namex(sb, root, name + 1, device);
    if (*ino != 0)
        inode = inode_get(sb, device, *ino);

    inode_release(root);
    return inode;
}

static int ax_open(struct file *file)
{
    struct ax_super_block sb;
    struct ax_inode *inode;
    uint32_t ino;

    get_super_block(&sb, file->f_inode->i_device);
    inode = namei(&sb, file->f_path, file->f_inode->i_device, &ino);

    if (!inode || inode->i_mode == AX_S_IFDIR)
    {
        inode_release(inode);
        return -1;
    }

    file->f_inode->i_ino = ino;
    file->f_inode->i_private = inode;
    return 0;
}

static int ax_close(struct file *file)
{
    inode_release(file->f_inode->i_private);
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
    .close      = ax_close,
    .read       = ax_read,
    .write      = ax_write
};

const struct file_system axfs =
{
    .name       = "axfs",
    .op         = &ops,
    .initialize = ax_fs_initialize
};
