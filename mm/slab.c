#include <mm/slab.h>
#include <mm/pmm.h>
#include <kernel/base.h>
#include <kernel/klib.h>
#include <lib/string.h>

#define VIRT_TO_PAGE(virt)  \
    (void *)((uint32_t)(virt) & ~(PAGE_SIZE - 1))

/* Each slab_page struct manage one physical memory page */
struct slab_page
{
    size_t avail;                   /* Available objects */
    size_t limit;                   /* Max objects in current slab_page */
    char *object_base;              /* Base address of object array */

    struct kmem_cache *cache;       /* Owner kmem_cache */
    struct slab_page *prev;
    struct slab_page *next;

    void *free[1];                  /* Free object pointer array in this
                                       slab_page, this array is dynamic
                                       allocated, array size is limit. */
};

/* Cache for kernel objects */
struct kmem_cache
{
    size_t object_size;             /* Object raw size */
    size_t size;                    /* Aligned object size */
    size_t align;                   /* Align size */

    struct slab_page full_slab;     /* No available object in this list */
    struct slab_page partial_slab;  /* Some available objects in this list */
    struct slab_page free_slab;     /* All objects are available in this list */

    struct kmem_cache *prev;
    struct kmem_cache *next;
};

#define KMEM_CACHE_SLAB_INIT(slab) \
    { 0, 0, NULL, NULL, slab, slab, { 0 } }

#define KMEM_CACHE_INIT(size, cache) \
    { \
        size, size, sizeof(void *), \
        KMEM_CACHE_SLAB_INIT(&cache.full_slab), \
        KMEM_CACHE_SLAB_INIT(&cache.partial_slab), \
        KMEM_CACHE_SLAB_INIT(&cache.free_slab), \
        &cache, &cache \
    }

#define KMALLOC_CACHE(size, index) \
    KMEM_CACHE_INIT(size, kmalloc_caches[index])

static struct kmem_cache kmem_cache_cache =
KMEM_CACHE_INIT(sizeof(kmem_cache_cache), kmem_cache_cache);

static struct kmem_cache kmalloc_caches[9] =
{
    KMALLOC_CACHE(4, 0),
    KMALLOC_CACHE(8, 1),
    KMALLOC_CACHE(16, 2),
    KMALLOC_CACHE(32, 3),
    KMALLOC_CACHE(64, 4),
    KMALLOC_CACHE(128, 5),
    KMALLOC_CACHE(256, 6),
    KMALLOC_CACHE(512, 7),
    KMALLOC_CACHE(1024, 8)
};

/* Slab list operations. */
static inline void slab_list_init(struct slab_page *head)
{
    head->next = head->prev = head;
}

static inline bool slab_list_empty(struct slab_page *head)
{
    return head->next == head;
}

static inline struct slab_page * slab_list_first(struct slab_page *head)
{
    return head->next;
}

static inline void slab_list_insert(struct slab_page *head,
                                    struct slab_page *slab)
{
    slab->prev = head;
    slab->next = head->next;
    head->next->prev = slab;
    head->next = slab;
}

static inline void slab_list_remove(struct slab_page *slab)
{
    slab->next->prev = slab->prev;
    slab->prev->next = slab->next;
    slab->prev = slab->next = NULL;
}

static inline void slab_list_destroy(struct slab_page *head)
{
    struct slab_page *slab = slab_list_first(head);
    while (slab != head)
    {
        /* Remove from list */
        struct slab_page *temp = slab;
        slab = slab->next;
        slab_list_remove(temp);

        /* Free physical memory page */
        pmm_free_page_address(CAST_VIRTUAL_TO_PHYSICAL(temp));
    }
}

static inline void init_slab_page(struct kmem_cache *cache,
                                  struct slab_page *slab)
{
    size_t payload = PAGE_SIZE - (sizeof(*slab) - sizeof(void *));

    /* Calculate capacity */
    slab->limit = payload / (cache->size + sizeof(void *));
    slab->object_base = (char *)slab + (PAGE_SIZE - (slab->limit * cache->size));
    slab->cache = cache;
    slab->avail = 0;

    /* Store all available objects address */
    for (size_t i = 0; i < slab->limit; ++i)
        slab->free[slab->avail++] = slab->object_base + i * cache->size;

    /* Insert into free_slab list */
    slab_list_insert(&cache->free_slab, slab);
}

static inline struct slab_page * alloc_slab_page(struct kmem_cache *cache)
{
    struct slab_page *slab = NULL;
    physical_addr_t page = pmm_alloc_page_address();
    if (!page)
        panic("[slab] - alloc a physical page failed.");

    slab = CAST_PHYSICAL_TO_VIRTUAL(page);
    init_slab_page(cache, slab);
    return slab;
}

static inline void * alloc_object_from_slab(struct slab_page *slab)
{
    void *object = NULL;
    if (slab->avail == 0)
        panic("[slab] - Fatal error, alloc object from full slab.");

    object = slab->free[--slab->avail];

    if (slab->avail == 0)
    {
        /* Move from partial slab list into full slab list. */
        slab_list_remove(slab);
        slab_list_insert(&slab->cache->full_slab, slab);
    }
    else if (slab->avail + 1 == slab->limit)
    {
        /* Move from free slab list into partial slab list. */
        slab_list_remove(slab);
        slab_list_insert(&slab->cache->partial_slab, slab);
    }

    return object;
}

static inline void * alloc_object_from_slab_list(struct slab_page *head)
{
    if (slab_list_empty(head))
        return NULL;
    return alloc_object_from_slab(slab_list_first(head));
}

static void * alloc_object(struct kmem_cache *cache)
{
    void *object = NULL;

    /* Alloc from partial slab first, free slab secondly */
    object = alloc_object_from_slab_list(&cache->partial_slab);
    if (!object)
        object = alloc_object_from_slab_list(&cache->free_slab);

    if (!object)
    {
        /*
         * No available object in free or partial slab list,
         * then we alloc a new slab.
         */
        struct slab_page *slab = alloc_slab_page(cache);
        object = alloc_object_from_slab(slab);
        if (!object)
            panic("[slab] - Fatal error, alloc object fail from a new slab.");
    }

    return object;
}

static void free_object(struct kmem_cache *cache, void *object)
{
    struct slab_page *slab = VIRT_TO_PAGE(object);
    if (slab->cache != cache)
        panic("[slab] - Fatal error, free object in wrong cache.");

    slab->free[slab->avail++] = object;

    if (slab->avail == 1 && slab->avail < slab->limit)
    {
        /* Move from full slab list into partial slab list. */
        slab_list_remove(slab);
        slab_list_insert(&cache->partial_slab, slab);
    }
    else if (slab->avail == slab->limit)
    {
        /* Move from partial slab list into free slab list. */
        slab_list_remove(slab);
        slab_list_insert(&cache->free_slab, slab);
    }
}

static inline struct kmem_cache * alloc_kmem_cache_object()
{
    return alloc_object(&kmem_cache_cache);
}

static inline void free_kmem_cache_object(struct kmem_cache *cache)
{
    free_object(&kmem_cache_cache, cache);
}

struct kmem_cache * slab_create_kmem_cache(size_t object_size, size_t align)
{
    struct kmem_cache *cache = alloc_kmem_cache_object();
    memset(cache, 0, sizeof(*cache));

    cache->object_size = object_size;
    cache->size = ALIGN(object_size, align);
    cache->align = align;

    /* Initialize slab lists. */
    slab_list_init(&cache->full_slab);
    slab_list_init(&cache->partial_slab);
    slab_list_init(&cache->free_slab);

    /* Insert into kmem_cache_cache list */
    cache->next = kmem_cache_cache.next;
    cache->prev = &kmem_cache_cache;
    kmem_cache_cache.next->prev = cache;
    kmem_cache_cache.next = cache;

    return cache;
}

void slab_destroy_kmem_cache(struct kmem_cache *cache)
{
    /* Remove cache from kmem_cache_cache list */
    cache->next->prev = cache->prev;
    cache->prev->next = cache->next;
    cache->next = cache->prev = NULL;

    /* Destroy slab lists. */
    slab_list_destroy(&cache->free_slab);
    slab_list_destroy(&cache->partial_slab);
    slab_list_destroy(&cache->full_slab);

    free_kmem_cache_object(cache);
}

void * slab_alloc(struct kmem_cache *cache)
{
    return alloc_object(cache);
}

void slab_free(struct kmem_cache *cache, void *object)
{
    free_object(cache, object);
}

static inline size_t kmalloc_index(size_t size)
{
    if (size <= 4) return 0;
    if (size <= 8) return 1;
    if (size <= 16) return 2;
    if (size <= 32) return 3;
    if (size <= 64) return 4;
    if (size <= 128) return 5;
    if (size <= 256) return 6;
    if (size <= 512) return 7;
    if (size <= 1024) return 8;
    panic("[BUG] kmalloc: can not alloc memory for more than 1024.");
    return 0;
}

void * kmalloc(size_t size)
{
    size_t index = kmalloc_index(size);
    struct kmem_cache *cache = &kmalloc_caches[index];
    return alloc_object(cache);
}

void kfree(void *ptr)
{
    struct slab_page *slab = VIRT_TO_PAGE(ptr);
    free_object(slab->cache, ptr);
}
