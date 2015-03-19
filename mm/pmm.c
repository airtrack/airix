#include <mm/pmm.h>
#include <kernel/klib.h>
#include <lib/stdlib.h>
#include <lib/string.h>

#define KERNEL_START_ADDRESS 0x100000
#define MEM_ALIGNMENT 4
#define PAGE_FLAG_USED 0x1
#define PAGE_FLAG_LOCK 0x2

#define ALIGN_ADDRESS(addr) \
    (void *)((((uint32_t)addr - 1) / MEM_ALIGNMENT + 1) * MEM_ALIGNMENT)

#define ALIGN_SIZE(size) \
    (((size - 1) / MEM_ALIGNMENT + 1) * MEM_ALIGNMENT)

#define ALIGN_PAGE_ADDRESS(addr) \
    (void *)((((uint32_t)addr - 1) / PAGE_SIZE + 1) * PAGE_SIZE)

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
    uint8_t order:4;                  /* Order number when in free_block_area */
};

/* Each free page block link with each other through page_block_node */
struct page_block_node
{
    struct page_block_node *prev;
    struct page_block_node *next;
};

/* Group same size page blocks */
struct free_block_area
{
    uint32_t num_blocks;              /* Number of free blocks */
    struct page_block_node free_head; /* Point to the first free page block */
};

/* All free memory pages */
struct free_mem_pages
{
    uint32_t num_pages;               /* Number of free pages */
    struct free_block_area free_areas[BUDDY_MAX_ORDER];
};

static struct boot_allocator boot_allocator;
static struct page *pages;
static struct free_mem_pages *free_mem;
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

static uint32_t calculate_total_pages(struct mmap_entry *entries, uint32_t num)
{
    uint64_t end = 0;
    for (uint32_t i = num; i > 0; --i)
    {
        if (entries[i - 1].type == PMM_MM_ENTRY_TYPE_NORMAL ||
            entries[i - 1].type == PMM_MM_ENTRY_TYPE_ACPI_REC)
        {
            end = (uint32_t)entries[i - 1].base + entries[i - 1].length;
            break;
        }
    }

    return end / PAGE_SIZE;
}

static void split_block_from_area(struct page *block, uint32_t order)
{
    struct page_block_node *node = PAGE_ADDRESS(block - pages);
    struct free_block_area *area = &free_mem->free_areas[order];

    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    node->prev = node->next = NULL;

    area->num_blocks--;
}

static void insert_block_into_area(struct page *block, uint32_t order)
{
    struct page_block_node *node = PAGE_ADDRESS(block - pages);
    struct free_block_area *area = &free_mem->free_areas[order];

    node->prev = &area->free_head;
    node->next = area->free_head.next;
    if (area->free_head.next)
        area->free_head.next->prev = node;
    area->free_head.next = node;

    area->num_blocks++;
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

static struct page * alloc_block_from_area(uint32_t free_order)
{
    struct free_block_area *area = &free_mem->free_areas[free_order];
    struct page_block_node *node = area->free_head.next;

    area->free_head.next = node->next;
    if (node->next)
        node->next->prev = &area->free_head;
    node->prev = node->next = NULL;

    area->num_blocks--;
    return pages + PAGE_NUMBER(node);
}

static inline struct page * split_buddy(struct page *block, uint32_t order)
{
    return block + order_pages[order];
}

static uint32_t alloc_block(uint32_t free_order, uint32_t order)
{
    struct page *block = alloc_block_from_area(free_order);

    while (free_order > order)
    {
        struct page *buddy = split_buddy(block, free_order - 1);
        buddy->flags = 0;
        insert_block_into_area(buddy, --free_order);
    }

    block->flags = PAGE_FLAG_USED;
    block->order = order;
    return block - pages;
}

uint32_t alloc_pages(uint32_t order)
{
    uint32_t check = order;
    while (check < BUDDY_MAX_ORDER)
    {
        if (free_mem->free_areas[check].num_blocks > 0)
        {
            uint32_t page_num = alloc_block(check, order);
            free_mem->num_pages -= order_pages[order];
            return page_num;
        }
        ++check;
    }

    return 0;
}

void free_pages(uint32_t page_num, uint32_t order)
{
    struct page *block = &pages[page_num];
    block->flags = 0;

    free_mem->num_pages += order_pages[order];

    for (; order < BUDDY_MAX_ORDER; ++order)
    {
        struct page *buddy = get_buddy(block, order);
        if (buddy && buddy->flags == 0 && buddy->order == order)
        {
            split_block_from_area(buddy, order);
            block = merge_buddy(block, buddy);
        }
        else
        {
            insert_block_into_area(block, order);
            break;
        }
    }
}

static void lock_kernel_pages()
{
    uint32_t kstart = KERNEL_START_ADDRESS / PAGE_SIZE;
    uint32_t kend =
        (uint32_t)ALIGN_PAGE_ADDRESS(boot_allocator.free_address) / PAGE_SIZE;

    for (uint32_t i = kstart; i < kend; ++i)
        pages[i].flags |= PAGE_FLAG_LOCK;
}

static void lock_boot_pages(void *boot_end)
{
    uint32_t num = (uint32_t)ALIGN_PAGE_ADDRESS(boot_end) / PAGE_SIZE;

    for (uint32_t i = 0; i < num; ++i)
        pages[i].flags |= PAGE_FLAG_LOCK;
}

static void init_pages(struct mmap_entry *entries, uint32_t num)
{
    uint32_t num_pages = calculate_total_pages(entries, num);

    /* Alloc all pages memory, set all pages has been used */
    pages = boot_alloc(sizeof(*pages) * num_pages);
    for (uint32_t i = 0; i < num_pages; ++i)
    {
        pages[i].flags = PAGE_FLAG_USED;
        pages[i].order = 0;
    }

    /* Alloc buddy memory struct */
    free_mem = boot_alloc(sizeof(*free_mem));
    memset(free_mem, 0, sizeof(*free_mem));

    /* Lock used memory pages */
    lock_boot_pages(entries + num);
    lock_kernel_pages();

    /* Init all free pages */
    for (uint32_t i = 0; i < num; ++i)
    {
        if (entries[i].type == PMM_MM_ENTRY_TYPE_NORMAL ||
            entries[i].type == PMM_MM_ENTRY_TYPE_ACPI_REC)
        {
            uint64_t start = (uint32_t)ALIGN_PAGE_ADDRESS(entries[i].base);
            uint64_t end = (uint32_t)entries[i].base + entries[i].length;
            for (; start + PAGE_SIZE <= end; start += PAGE_SIZE)
            {
                uint32_t page_num = PAGE_NUMBER(start);
                if (!(pages[page_num].flags & PAGE_FLAG_LOCK))
                    free_pages(page_num, 0);
            }
        }
    }

    printk("Total pages: %u, free pages: %u\n", num_pages, free_mem->num_pages);
}

void init_pmm(void *free_addr, struct mmap_entry *entries, uint32_t num)
{
    qsort(entries, num, sizeof(*entries), mmap_entry_compare);
    for (uint32_t i = 0; i < num; ++i)
    {
        printk("%d: %p - %p %d %d 0x%x\n", i, entries[i].base,
                (char *)entries[i].base + entries[i].length,
                entries[i].length, entries[i].type, entries[i].acpi_attr);
    }

    init_boot_allocator(free_addr);
    init_pages(entries, num);
}
