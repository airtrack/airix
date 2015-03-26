#include <mm/pmm.h>
#include <kernel/klib.h>
#include <lib/stdlib.h>
#include <lib/string.h>

#define KERNEL_START_ADDRESS 0x100000

enum page_flag
{
    PAGE_FLAG_USED = 0x1,
    PAGE_FLAG_LOCK = 0x2,
};

/* Allocator provide memory allocation before MM initialized.
 * Only support memory alloc, do not support memory free. */
struct boot_allocator
{
    char *start_address;
    char *free_address;
};

/* Each page has a corresponding struct page object */
struct page
{
    uint8_t flags:4;
    uint8_t order:4;                  /* Order number of block */
};

/* Each free page block link with each other through page_block_node */
struct page_block_node
{
    struct page_block_node *prev;
    struct page_block_node *next;
};

/* Group the same size page blocks */
struct free_block_area
{
    uint32_t num_blocks;              /* Number of free blocks */
    struct page_block_node free_head; /* Sentry node */
};

/* All free memory */
struct free_blocks
{
    uint32_t total_pages;
    uint32_t num_pages;               /* Number of free pages */
    struct free_block_area free_areas[BUDDY_MAX_ORDER];
};

static struct boot_allocator boot_allocator;
static struct page *pages;
static struct free_blocks *free_blocks;
static const uint32_t order_pages[BUDDY_MAX_ORDER] =
{ 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };

static void init_boot_allocator(void *start_address)
{
    start_address = ALIGN_ADDRESS(start_address);
    boot_allocator.start_address = start_address;
    boot_allocator.free_address = start_address;
}

static inline void * boot_alloc(uint32_t size)
{
    void *address = boot_allocator.free_address;
    boot_allocator.free_address += ALIGN_SIZE(size);
    return address;
}

static int mmap_entry_compare(const void *left, const void *right)
{
    const struct mmap_entry *l = (const struct mmap_entry *)left;
    const struct mmap_entry *r = (const struct mmap_entry *)right;
    if (l->base < r->base) return -1;
    else if (l->base == r->base) return 0;
    else return 1;
}

static void init_pages_data(uint32_t num_pages)
{
    /* Alloc all pages memory, set all pages has been used */
    pages = boot_alloc(sizeof(*pages) * num_pages);

    for (uint32_t i = 0; i < num_pages; ++i)
    {
        pages[i].flags = PAGE_FLAG_USED;
        pages[i].order = 0;
    }
}

static void init_free_blocks(uint32_t total_pages)
{
    free_blocks = boot_alloc(sizeof(*free_blocks));

    /* Set each area empty */
    for (uint32_t i = 0; i < BUDDY_MAX_ORDER; ++i)
    {
        struct free_block_area *area = &free_blocks->free_areas[i];
        area->free_head.prev = &area->free_head;
        area->free_head.next = &area->free_head;
        area->num_blocks = 0;
    }

    free_blocks->total_pages = total_pages;
    free_blocks->num_pages = 0;
}

/* Require memory map entries have been sorted */
static uint64_t get_max_physical_address(struct mmap_entry *entries,
                                         uint32_t num)
{
    uint64_t end = 0;

    for (uint32_t i = num; i > 0; --i)
    {
        if (entries[i - 1].type == PMM_MM_ENTRY_TYPE_NORMAL ||
            entries[i - 1].type == PMM_MM_ENTRY_TYPE_ACPI_REC)
        {
            end = (uint64_t)entries[i - 1].base + entries[i - 1].length;
            break;
        }
    }

    return end;
}

static inline uint32_t calculate_total_pages(struct mmap_entry *entries,
                                             uint32_t num)
{
    return get_max_physical_address(entries, num) / PAGE_SIZE;
}

static void split_block_from_area(struct page *block, uint32_t order)
{
    physical_addr_t addr = PAGE_ADDRESS(block - pages);
    struct page_block_node *node = CAST_PHYSICAL_TO_VIRTUAL(addr);
    struct free_block_area *area = &free_blocks->free_areas[order];

    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node->next = NULL;

    area->num_blocks--;
}

static void insert_block_into_area(struct page *block, uint32_t order)
{
    physical_addr_t addr = PAGE_ADDRESS(block - pages);
    struct page_block_node *node = CAST_PHYSICAL_TO_VIRTUAL(addr);
    struct free_block_area *area = &free_blocks->free_areas[order];

    node->next = area->free_head.next;
    node->prev = &area->free_head;
    area->free_head.next->prev = node;
    area->free_head.next = node;

    area->num_blocks++;
    block->flags = 0;
    block->order = order;
}

static inline struct page * get_buddy(struct page *block, uint32_t order)
{
    if (order + 1 >= BUDDY_MAX_ORDER)
        return NULL;

    if ((block - pages) % order_pages[order + 1] == 0)
        return block + order_pages[order];
    else
        return block - order_pages[order];
}

static inline struct page * merge_buddy(
        struct page *buddy1, struct page *buddy2)
{
    if (buddy1 < buddy2)
        return buddy1;
    else
        return buddy2;
}

static inline struct page * split_buddy(struct page *block, uint32_t order)
{
    return block + order_pages[order];
}

static struct page * alloc_block_from_area(uint32_t free_order)
{
    struct free_block_area *area = &free_blocks->free_areas[free_order];
    struct page_block_node *node = area->free_head.next;
    physical_addr_t node_addr = CAST_VIRTUAL_TO_PHYSICAL(node);

    /* No block in this area */
    if (node == &area->free_head)
        return NULL;

    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node->next = NULL;

    area->num_blocks--;
    return pages + PAGE_NUMBER(node_addr);
}

static uint32_t alloc_block(uint32_t free_order, uint32_t order)
{
    struct page *block = alloc_block_from_area(free_order);

    while (free_order > order)
    {
        /* Split block when the order of block is large than needed,
         * returns the splitted buddy block back to the free blocks. */
        struct page *buddy = split_buddy(block, free_order - 1);
        insert_block_into_area(buddy, --free_order);
    }

    block->flags = PAGE_FLAG_USED;
    block->order = order;
    return block - pages;
}

uint32_t pmm_alloc_pages(uint32_t order)
{
    for (uint32_t check = order; check < BUDDY_MAX_ORDER; ++check)
    {
        /* Find a non empty area */
        if (free_blocks->free_areas[check].num_blocks > 0)
        {
            uint32_t page_num = alloc_block(check, order);
            free_blocks->num_pages -= order_pages[order];
            return page_num;
        }
    }

    return 0;
}

void pmm_free_pages(uint32_t page_num, uint32_t order)
{
    if (page_num == 0)
        return ;

    struct page *block = &pages[page_num];
    block->flags = 0;

    free_blocks->num_pages += order_pages[order];

    for (; order < BUDDY_MAX_ORDER; ++order)
    {
        struct page *buddy = get_buddy(block, order);
        if (buddy && buddy->flags == 0 && buddy->order == order)
        {
            /* If buddy is free too, then merge with buddy */
            split_block_from_area(buddy, order);
            block = merge_buddy(block, buddy);
        }
        else
        {
            insert_block_into_area(block, order);
            break;
        }
    }

    if (order == BUDDY_MAX_ORDER)
        insert_block_into_area(block, order - 1);
}

static void lock_boot_pages(void *boot_end)
{
    /* Lock [page 0, page of boot end address) */
    physical_addr_t end = CAST_VIRTUAL_TO_PHYSICAL(boot_end);
    uint32_t num = PAGE_NUMBER(ALIGN_PAGE(end));

    for (uint32_t i = 0; i < num; ++i)
        pages[i].flags |= PAGE_FLAG_LOCK;
}

static void lock_kernel_pages()
{
    /* Lock [page of kernel start, page of boot free address) */
    physical_addr_t end = CAST_VIRTUAL_TO_PHYSICAL(
        boot_allocator.free_address);

    uint32_t kstart = PAGE_NUMBER(KERNEL_START_ADDRESS);
    uint32_t kend = PAGE_NUMBER(ALIGN_PAGE(end));

    for (uint32_t i = kstart; i < kend; ++i)
        pages[i].flags |= PAGE_FLAG_LOCK;
}

static void init_free_pages(struct mmap_entry *entries, uint32_t num)
{
    /* Free all unlocked pages */
    for (uint32_t i = 0; i < num; ++i)
    {
        if (entries[i].type == PMM_MM_ENTRY_TYPE_NORMAL ||
            entries[i].type == PMM_MM_ENTRY_TYPE_ACPI_REC)
        {
            uint64_t start = ALIGN_PAGE(entries[i].base);
            uint64_t end = (uint64_t)entries[i].base + entries[i].length;
            for (; start + PAGE_SIZE <= end; start += PAGE_SIZE)
            {
                uint32_t page_num = PAGE_NUMBER(start);
                if (!(pages[page_num].flags & PAGE_FLAG_LOCK))
                    pmm_free_pages(page_num, 0);
            }
        }
    }
}

static void init_pages(struct mmap_entry *entries, uint32_t num)
{
    uint32_t num_pages = calculate_total_pages(entries, num);

    /* Init page array */
    init_pages_data(num_pages);

    /* Init buddy memory struct */
    init_free_blocks(num_pages);

    /* Lock used memory pages */
    lock_boot_pages(entries + num);
    lock_kernel_pages();

    /* Init all free pages */
    init_free_pages(entries, num);
}

void pmm_initialize(physical_addr_t free_addr, struct mmap_entry *entries,
                    uint32_t num)
{
    qsort(entries, num, sizeof(*entries), mmap_entry_compare);
    init_boot_allocator(CAST_PHYSICAL_TO_VIRTUAL(free_addr));
    init_pages(entries, num);
}

uint64_t pmm_max_physical_address(struct mmap_entry *entries, uint32_t num)
{
    qsort(entries, num, sizeof(*entries), mmap_entry_compare);
    return get_max_physical_address(entries, num);
}

void pmm_print_statistics(struct mmap_entry *entries, uint32_t num)
{
    if (entries)
    {
        for (uint32_t i = 0; i < num; ++i)
        {
            printk("%d: %p - %p %d %d 0x%x\n", i,
                   entries[i].base,
                   entries[i].base + entries[i].length,
                   entries[i].length, entries[i].type,
                   entries[i].acpi_attr);
        }
    }

    printk("Total pages: %u, free pages: %u\n",
           free_blocks->total_pages, free_blocks->num_pages);
}
