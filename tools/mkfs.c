#include <fs/axfs.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_IMG_NAME "axfs.img"
#define ALIGN_4(size) (((size) + 4) & ~(4 - 1))

struct ax_super_block super_block;

void get_block_group_metadata(FILE *img, uint32_t bg_no,
                              struct ax_block_group_descriptor *desc)
{
    long offset = bg_no * sizeof(*desc) +
        (AX_FS_SUPER_BLOCK_NO + 1) * AX_FS_BLOCK_SIZE;

    fseek(img, offset, SEEK_SET);
    fread(desc, 1, sizeof(*desc), img);
}

void update_block_group_metadata(FILE *img, uint32_t bg_no,
                                 const struct ax_block_group_descriptor *desc)
{
    long offset = bg_no * sizeof(*desc) +
        (AX_FS_SUPER_BLOCK_NO + 1) * AX_FS_BLOCK_SIZE;

    fseek(img, offset, SEEK_SET);
    fwrite(desc, 1, sizeof(*desc), img);
}

void update_inode_bitmap(FILE *img, uint32_t inode_bitmap, uint32_t bg_ino)
{
    uint32_t byte = bg_ino / 8;
    uint32_t bit = (8 - bg_ino % 8) - 1;
    uint8_t bits;

    fseek(img, inode_bitmap * AX_FS_BLOCK_SIZE + byte, SEEK_SET);
    fread(&bits, 1, sizeof(bits), img);

    bits |= (1 << bit);

    fseek(img, -1, SEEK_CUR);
    fwrite(&bits, 1, sizeof(bits), img);
}

void write_inode(FILE *img, const struct ax_inode *inode, uint32_t ino)
{
    struct ax_block_group_descriptor desc;
    uint32_t bg = (ino - 1) / super_block.s_inodes_per_group;
    uint32_t bg_ino = (ino - 1) % super_block.s_inodes_per_group;
    long offset;

    get_block_group_metadata(img, bg, &desc);

    offset = bg_ino * sizeof(*inode) + desc.bg_inode_table * AX_FS_BLOCK_SIZE;
    fseek(img, offset, SEEK_SET);
    fwrite(inode, 1, sizeof(*inode), img);

    update_inode_bitmap(img, desc.bg_inode_bitmap, bg_ino);

    desc.bg_free_inodes_count -= 1;
    update_block_group_metadata(img, bg, &desc);
}

uint16_t write_dir_entry(FILE *img, const char *name,
                         uint8_t ft, uint32_t ino)
{
    struct ax_directory_entry *entry;
    uint8_t name_len = strlen(name);
    uint16_t rec_len = ALIGN_4(sizeof(*entry) + name_len);

    entry = malloc(rec_len);
    if (!entry)
    {
        fprintf(stderr, "malloc failed.\n");
        exit(1);
    }

    memset(entry, 0, rec_len);

    entry->inode = ino;
    entry->rec_len = rec_len;
    entry->name_len = name_len;
    entry->file_type = ft;
    memcpy(entry->name, name, name_len);

    fwrite(entry, 1, rec_len, img);
    free(entry);
    return rec_len;
}

void write_padding_entry(FILE *img, uint16_t padding)
{
    struct ax_directory_entry *entry;

    if (padding < sizeof(*entry))
        return ;

    entry = malloc(padding);
    if (!entry)
    {
        fprintf(stderr, "malloc failed.\n");
        exit(1);
    }

    memset(entry, 0, padding);
    entry->rec_len = padding;

    fwrite(entry, 1, padding, img);
    free(entry);
}

uint32_t alloc_block_from_block_group(FILE *img, uint32_t bg_no)
{
    struct ax_block_group_descriptor desc;
    get_block_group_metadata(img, bg_no, &desc);

    if (desc.bg_free_blocks_count > 0)
    {
        uint8_t bitmap[AX_FS_BLOCK_SIZE];
        fseek(img, desc.bg_block_bitmap * AX_FS_BLOCK_SIZE, SEEK_SET);
        fread(bitmap, 1, sizeof(bitmap), img);

        for (int i = 0; i < AX_FS_BLOCK_SIZE; ++i)
        {
            if (bitmap[i] != 0xFF)
            {
                for (int b = 0; b < 8; ++b)
                {
                    if (!(bitmap[i] & (1 << (7 - b))))
                    {
                        return b + i * 8 + bg_no * AX_FS_BLOCKS_PER_GROUP +
                            AX_FS_SUPER_BLOCK_NO;
                    }
                }
            }
        }
    }

    return 0;
}

uint32_t alloc_block(FILE *img)
{
    uint32_t bg_num = super_block.s_blocks_count / AX_FS_BLOCKS_PER_GROUP +
        (super_block.s_blocks_count % AX_FS_BLOCKS_PER_GROUP) ? 1 : 0;

    for (uint32_t i = 0; i < bg_num; ++i)
    {
        uint32_t block = alloc_block_from_block_group(img, i);

        if (block > 0 &&
            block < super_block.s_blocks_count + AX_FS_SUPER_BLOCK_NO)
            return block;
    }

    fprintf(stderr, "Alloc block failed.\n");
    exit(1);
    return 0;
}

void update_block_bitmap(FILE *img, uint32_t block)
{
    struct ax_block_group_descriptor desc;
    uint32_t bg = (block - AX_FS_SUPER_BLOCK_NO) / AX_FS_BLOCKS_PER_GROUP;
    uint32_t bg_bno = (block - AX_FS_SUPER_BLOCK_NO) % AX_FS_BLOCKS_PER_GROUP;
    uint32_t byte = bg_bno / 8;
    uint32_t bit = (8 - bg_bno % 8) - 1;
    uint8_t bits;

    get_block_group_metadata(img, bg, &desc);
    fseek(img, desc.bg_block_bitmap * AX_FS_BLOCK_SIZE + byte, SEEK_SET);
    fread(&bits, 1, sizeof(bits), img);

    bits |= (1 << bit);

    fseek(img, -1, SEEK_CUR);
    fwrite(&bits, 1, sizeof(bits), img);
}

void write_super_block(FILE *img)
{
    fseek(img, AX_FS_SUPER_BLOCK_NO * AX_FS_BLOCK_SIZE, SEEK_SET);
    fwrite(&super_block, 1, sizeof(super_block), img);
}

void update_super_block(FILE *img)
{
    write_super_block(img);
}

void write_root_dir(FILE *img)
{
    struct ax_inode inode;
    uint16_t padding_len = AX_FS_BLOCK_SIZE;
    uint32_t block = alloc_block(img);

    fseek(img, block * AX_FS_BLOCK_SIZE, SEEK_SET);

    /* '.' entry */
    padding_len -= write_dir_entry(img, ".", AX_FT_DIR, AX_FS_ROOT_INO);

    /* '..' entry */
    padding_len -= write_dir_entry(img, "..", AX_FT_DIR, AX_FS_ROOT_INO);

    /* Padding entry */
    write_padding_entry(img, padding_len);

    memset(&inode, 0, sizeof(inode));

    inode.i_mode = AX_S_IFDIR;
    inode.i_size = AX_FS_BLOCK_SIZE - padding_len;
    inode.i_block[0] = block;
    write_inode(img, &inode, AX_FS_ROOT_INO);

    update_block_bitmap(img, block);

    super_block.s_free_blocks_count -= 1;
    super_block.s_free_inodes_count -= 1;
    update_super_block(img);
}

void write_block_group_metadata(FILE *img, uint32_t block_group_no,
                                uint32_t used_blocks,
                                const struct ax_block_group_descriptor *desc)
{
    unsigned char bits = 0xFF;
    long offset = block_group_no * sizeof(*desc) +
        (AX_FS_SUPER_BLOCK_NO + 1) * AX_FS_BLOCK_SIZE;

    /* Write block group descriptor */
    fseek(img, offset, SEEK_SET);
    fwrite(desc, 1, sizeof(*desc), img);

    /* Write block bitmap */
    offset = desc->bg_block_bitmap * AX_FS_BLOCK_SIZE;
    fseek(img, offset, SEEK_SET);

    for (uint32_t i = 0; i < used_blocks / 8; ++i)
        fwrite(&bits, 1, sizeof(bits), img);

    bits <<= (8 - used_blocks % 8);
    fwrite(&bits, 1, sizeof(bits), img);

    /* Update super block */
    super_block.s_free_inodes_count += desc->bg_free_inodes_count;
    super_block.s_free_blocks_count += desc->bg_free_blocks_count;
}

void write_blocks(FILE *img, long count)
{
    char c = 0;
    fseek(img, count * AX_FS_BLOCK_SIZE - 1, SEEK_END);
    fwrite(&c, 1, sizeof(c), img);
}

void make_unused_blocks(FILE *img)
{
    write_blocks(img, 1);
}

void make_first_block_group(FILE *img, uint32_t block_groups,
                            uint32_t inodes_per_group, uint32_t blocks)
{
    /*
     * The first block group structure:
     * superblock
     * block group descriptor table
     * block bitmap block
     * inode bitmap block
     * inode table
     * data blocks
     */
    struct ax_block_group_descriptor bg_descriptor;

    uint32_t bg_table_size = block_groups *
        sizeof(struct ax_block_group_descriptor);
    uint32_t bg_table_blocks = bg_table_size / AX_FS_BLOCK_SIZE +
        (bg_table_size % AX_FS_BLOCK_SIZE ? 1 : 0);

    uint32_t inodes_table_size = inodes_per_group
        * sizeof(struct ax_inode);
    uint32_t inodes_table_blocks = inodes_table_size / AX_FS_BLOCK_SIZE +
        (inodes_table_size % AX_FS_BLOCK_SIZE ? 1 : 0);

    uint32_t used_blocks = 1 + bg_table_blocks + 1 + 1 + inodes_table_blocks;

    if (blocks <= used_blocks)
    {
        fprintf(stderr, "Could not make the first block group.\n");
        exit(1);
    }

    /* Superblock */
    write_blocks(img, 1);

    /* Block group descriptor table */
    write_blocks(img, bg_table_blocks);

    /* Block bitmap */
    write_blocks(img, 1);

    /* Inode bitmap */
    write_blocks(img, 1);

    /* Inodes table */
    write_blocks(img, inodes_table_blocks);

    /* Data blocks */
    write_blocks(img, blocks - used_blocks);

    bg_descriptor.bg_block_bitmap = AX_FS_SUPER_BLOCK_NO + 1 + bg_table_blocks;
    bg_descriptor.bg_inode_bitmap = bg_descriptor.bg_block_bitmap + 1;
    bg_descriptor.bg_inode_table = bg_descriptor.bg_inode_bitmap + 1;
    bg_descriptor.bg_free_blocks_count = blocks - used_blocks;
    bg_descriptor.bg_free_inodes_count = inodes_per_group;

    write_block_group_metadata(img, 0, used_blocks, &bg_descriptor);
}

void make_block_group(FILE *img, uint32_t start_block, uint32_t block_group_no,
                      uint32_t inodes_per_group, uint32_t blocks)
{
    /*
     * Block group structure:
     * block bitmap block
     * inode bitmap block
     * inode table
     * data blocks
     */
    struct ax_block_group_descriptor bg_descriptor;
    uint32_t inodes_table_size = inodes_per_group * sizeof(struct ax_inode);
    uint32_t inodes_table_blocks = inodes_table_size / AX_FS_BLOCK_SIZE +
        (inodes_table_size % AX_FS_BLOCK_SIZE ? 1 : 0);
    uint32_t used_blocks = 1 + 1 + inodes_table_blocks;

    if (blocks <= used_blocks)
    {
        fprintf(stderr, "Could not make %u block group.\n", block_group_no);
        exit(1);
    }

    /* Block bitmap */
    write_blocks(img, 1);

    /* Inode bitmap */
    write_blocks(img, 1);

    /* Inodes table */
    write_blocks(img, inodes_table_blocks);

    /* Data blocks */
    write_blocks(img, blocks - used_blocks);

    bg_descriptor.bg_block_bitmap = start_block;
    bg_descriptor.bg_inode_bitmap = bg_descriptor.bg_block_bitmap + 1;
    bg_descriptor.bg_inode_table = bg_descriptor.bg_inode_bitmap + 1;
    bg_descriptor.bg_free_blocks_count = blocks - used_blocks;
    bg_descriptor.bg_free_inodes_count = inodes_per_group;

    write_block_group_metadata(img, block_group_no,
                               used_blocks, &bg_descriptor);
}

void mkfs(const char *img_path, uint32_t block_num, uint32_t inodes_per_group)
{
    FILE *img;
    uint32_t start_block;
    uint32_t block_groups;

    /* The first block is reserved. */
    block_num -= 1;
    block_groups = block_num / AX_FS_BLOCKS_PER_GROUP;
    block_groups += block_num % AX_FS_BLOCKS_PER_GROUP ? 1 : 0;

    if (block_groups < 1)
    {
        fprintf(stderr, "FS '%s' size is too small.\n", img_path);
        exit(1);
    }

    img = fopen(img_path, "w+");
    if (!img)
    {
        fprintf(stderr, "Could not open img: %s.\n", img_path);
        exit(1);
    }

    /* Update super block */
    super_block.s_inodes_count = inodes_per_group * block_groups;
    super_block.s_blocks_count = block_num;
    super_block.s_inodes_per_group = inodes_per_group;

    make_unused_blocks(img);

    start_block = AX_FS_SUPER_BLOCK_NO;

    for (uint32_t i = 0; i < block_groups; ++i)
    {
        uint32_t blocks = block_num > AX_FS_BLOCKS_PER_GROUP ?
            AX_FS_BLOCKS_PER_GROUP : block_num;

        if (i == 0)
        {
            make_first_block_group(img, block_groups,
                                   inodes_per_group, blocks);
        }
        else
        {
            make_block_group(img, start_block, i, inodes_per_group, blocks);
        }

        block_num -= blocks;
        start_block += blocks;
    }

    write_super_block(img);
    write_root_dir(img);

    fclose(img);
}

void usage()
{
    fprintf(stderr, "\
Options:\n\
    -n      Specify fs name.\n\
    -s      Specify fs size in MB.\n\
    -i      Specify inode number of each block group.\n");

    exit(1);
}

static char * dupstr(const char *s)
{
    int n = strlen(s);
    char *dup = malloc(n + 1);

    if (dup)
    {
        strncpy(dup, s, n);
        dup[n] = '\0';
    }

    return dup;
}

int main(int argc, char **argv)
{
    int ch;
    char *name = NULL;
    unsigned long long fs_size = 0;
    unsigned long long fs_inodes_per_group = 1712;

    if (argc < 2)
    {
        usage();
    }

    while ((ch = getopt(argc, argv, "n:s:i:")) != -1)
    {
        switch (ch)
        {
        case 'n':
            name = dupstr(optarg);
            break;

        case 's':
            fs_size = strtoull(optarg, NULL, 10);
            break;

        case 'i':
            fs_inodes_per_group = strtoull(optarg, NULL, 10);
            break;

        default:
            usage();
        }
    }

    printf("'%s' size is %llu MB, %llu inodes per block group.\n",
           name ? name : DEFAULT_IMG_NAME, fs_size, fs_inodes_per_group);

    if (fs_size == 0 || fs_inodes_per_group == 0)
    {
        fprintf(stderr, "\narguments error: size or inodes could not be 0.\n");
        return 1;
    }

    fs_size *= 1024 * 1024 / AX_FS_BLOCK_SIZE;
    mkfs(name ? name : DEFAULT_IMG_NAME, fs_size, fs_inodes_per_group);
    free(name);
    return 0;
}
