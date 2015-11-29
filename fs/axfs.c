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

static inline const char * str_find(const char *str, int c)
{
    while (*str != '\0' && (unsigned char)*str != c)
        ++str;
    return str;
}

static inline struct bio * block_read(uint8_t device, uint32_t block)
{
    struct bio *bio = bio_get(device, SECTOR_NO(block));

    if (bio)
    {
        if (!bio_read(bio))
        {
            bio_release(bio);
            bio = NULL;
        }
    }

    return bio;
}

static inline void block_release(struct bio *bio)
{
    bio_release(bio);
}

static uint32_t block_find_ino(const char *name, size_t length,
                               uint8_t device, uint32_t block)
{
    uint32_t ino = 0;
    struct bio *bio = block_read(device, SECTOR_NO(block));

    if (bio)
    {
        char *it = bio_data(bio);
        char *end = it + AX_FS_BLOCK_SIZE;
        struct ax_directory_entry *entry = NULL;

        for (; it < end; it += entry->rec_len)
        {
            entry = (struct ax_directory_entry *)(it);
            if (entry->name_len == length &&
                memcmp(name, entry->name, length) == 0)
            {
                ino = entry->inode;
                break;
            }
        }

        block_release(bio);
    }

    return ino;
}

static uint32_t dir_find_ino(const struct ax_inode *dir,
                             const char *name, size_t length,
                             uint8_t device)
{
    uint32_t ino = 0;

    for (int i = 0; i < AX_FS_DIRECT_BLOCK_COUNT; ++i)
    {
        if (dir->i_block[i] == 0)
            break;

        ino = block_find_ino(name, length, device, dir->i_block[i]);
        if (ino != 0)
            break;
    }

    /* TODO: Find in indirect blocks */

    return ino;
}

static uint32_t namex(const struct ax_super_block *sb,
                      const char *name, uint8_t device,
                      uint32_t ino)
{
    const char *begin = name;
    const char *end = str_find(begin, '/');
    struct ax_inode *inode = NULL;

    while (end - begin != 0)
    {
        inode = inode_get(sb, device, ino);

        if (!inode || inode->i_mode != AX_S_IFDIR)
        {
            ino = 0;
            break;
        }

        ino = dir_find_ino(inode, begin, end - begin, device);

        if (ino == 0)
            break;

        inode_release(inode);
        inode = NULL;

        if (*end == '\0')
        {
            begin = end;
        }
        else
        {
            begin = end + 1;
            end = str_find(begin, '/');
        }
    }

    inode_release(inode);
    return ino;
}

static struct ax_inode * namei(const struct ax_super_block *sb,
                               const char *name, uint8_t device,
                               uint32_t *ino)
{
    struct ax_inode *inode = NULL;

    if (*name == '/')
    {
        *ino = namex(sb, name + 1, device, AX_FS_ROOT_INO);
        if (*ino != 0)
            inode = inode_get(sb, device, *ino);
    }

    return inode;
}

static int read_file(struct inode *inode, uint64_t pos,
                     char *buffer, size_t size)
{
    struct ax_inode *ax_inode = inode->i_private;
    uint32_t block_index = pos / AX_FS_BLOCK_SIZE;
    uint32_t block_pos = pos % AX_FS_BLOCK_SIZE;
    char *begin = buffer;

    /* EOF */
    if (pos >= ax_inode->i_size)
        return 0;

    if (block_index >= AX_FS_DIRECT_BLOCK_COUNT)
        return -1;

    size = KMIN(size, ax_inode->i_size - pos);
    while (size > 0 && block_index < AX_FS_DIRECT_BLOCK_COUNT)
    {
        uint32_t bytes = 0;
        struct bio *block = block_read(inode->i_device,
                                       ax_inode->i_block[block_index]);
        if (!block)
            return -1;

        bytes = KMIN(size, AX_FS_BLOCK_SIZE - block_pos);
        memcpy(buffer, bio_data(block) + block_pos, bytes);
        block_release(block);

        size -= bytes;
        buffer += bytes;

        block_index += 1;
        block_pos = 0;
    }

    /* TODO: read from indirect blocks */

    return buffer - begin;
}

static int ax_open(struct file *file)
{
    struct ax_super_block sb;
    struct ax_inode *inode;
    uint32_t ino;

    if (file->f_flags != O_RDONLY)
        return -1;

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
    int read = 0;

    if (file->f_flags != O_RDONLY || file->f_flags != O_RDWR)
        return -1;

    read = read_file(file->f_inode, file->f_pos, buffer, size);

    if (read > 0)
        file->f_pos += (uint64_t)read;

    return read;
}

static int ax_write(struct file *file, const char *buffer, size_t size)
{
    (void)file, (void)buffer, (void)size;
    return -1;
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
