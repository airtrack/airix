#ifndef AXFS_H
#define AXFS_H

#include <kernel/ide.h>
#include <stdint.h>

#define AX_FS_ROOT_INO 1
#define AX_FS_SUPER_BLOCK_NO 1
#define AX_FS_BLOCK_SIZE 1024
#define AX_FS_BLOCKS_PER_GROUP (8 * AX_FS_BLOCK_SIZE)
#define AX_FS_SECTORS_PER_BLOCK (AX_FS_BLOCK_SIZE / SECTOR_SIZE)

struct ax_super_block
{
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_free_blocks_count;
    uint32_t s_inodes_per_group;
};

struct ax_block_group_descriptor
{
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
};

enum ax_inode_mode
{
    AX_S_IFREG = 0x8000,
    AX_S_IFDIR = 0x4000,
};

struct ax_inode
{
    uint16_t i_mode;
    uint16_t i_uid;
    uint64_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_block[24];
};

enum ax_file_type
{
    AX_FT_UNKNOWN = 0,
    AX_FT_REG_FILE = 1,
    AX_FT_DIR = 2,
};

struct ax_directory_entry
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    uint8_t name[0];
};

#endif /* AXFS_H */
