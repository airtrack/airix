#include <kernel/bio.h>
#include <kernel/klib.h>
#include <kernel/ide.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <mm/slab.h>
#include <mm/pmm.h>
#include <string.h>

#define MAX_CACHE_COUNT 4096
#define SECTORS_PER_BIO (PAGE_SIZE / SECTOR_SIZE)
#define BASE_SECTOR(sector) ((sector) / SECTORS_PER_BIO)

enum bio_flag
{
    BIO_FLAG_UPDATED    = 1,
    BIO_FLAG_DIRTY      = 1 << 1,
    BIO_FLAG_REFFED     = 1 << 2,
};

struct bio
{
    void *buffer;           /* Buffer of bio */
    uint8_t flag;           /* Flags of bio */
    uint8_t iter;           /* Sector iterator */
    uint8_t dev;            /* Device ID */
    uint64_t sector;        /* Base sector */

    struct process *sleep;  /* Sleep process of waiting the bio */
    struct bio *prev;
    struct bio *next;
};

static struct kmem_cache *bio_cache;
static struct bio bio_cache_head;
static int bio_cache_count;

static inline void split_bio_node(struct bio *bio)
{
    if (bio->prev)
        bio->prev->next = bio->next;
    if (bio->next)
        bio->next->prev = bio->prev;
    bio->prev = bio->next = NULL;
}

static inline void insert_into_list_head(struct bio *bio)
{
    bio->next = bio_cache_head.next;
    bio->prev = &bio_cache_head;
    bio_cache_head.next->prev = bio;
    bio_cache_head.next = bio;
}

void bio_initialize()
{
    bio_cache_head.next = &bio_cache_head;
    bio_cache_head.prev = &bio_cache_head;

    bio_cache = slab_create_kmem_cache(
        sizeof(struct bio), sizeof(void *));
    if (!bio_cache)
        panic("bio slab initialize failed");
}

static struct bio * find_bio(uint8_t dev, uint64_t sector)
{
    struct bio *cache = bio_cache_head.next;

    while (cache != &bio_cache_head)
    {
        if (cache->dev == dev && cache->sector == sector)
            break;

        cache = cache->next;
    }

    return cache == &bio_cache_head ? NULL : cache;
}

static struct bio * find_unused_bio()
{
    struct bio *cache = bio_cache_head.prev;

    while (cache != &bio_cache_head)
    {
        if (!(cache->flag & BIO_FLAG_REFFED))
            break;

        cache = cache->prev;
    }

    return cache == &bio_cache_head ? NULL : cache;
}

static struct bio * slab_alloc_bio()
{
    struct bio *bio = slab_alloc(bio_cache);

    if (bio)
    {
        memset(bio, 0, sizeof(*bio));
        bio->buffer = cast_p2v_or_null(pmm_alloc_page());

        if (bio->buffer)
        {
            ++bio_cache_count;
        }
        else
        {
            slab_free(bio_cache, bio);
            bio = NULL;
        }
    }

    return bio;
}

static void sleep_wait_bio(struct bio *bio)
{
    struct process *proc = sched_get_running_proc();

    /* Insert into the sleep process list */
    if (bio->sleep)
    {
        struct process *sleep = bio->sleep;
        while (sleep->sleep)
            sleep = sleep->sleep;
        sleep->sleep = proc;
    }
    else
    {
        /* The first sleep process */
        bio->sleep = proc;
    }

    proc->state = PROC_STATE_IO;
    sched();
}

static struct bio * alloc_bio()
{
    struct bio *cache = NULL;

    while (!cache)
    {
        /* Alloc from slab */
        if (bio_cache_count < MAX_CACHE_COUNT)
            cache = slab_alloc_bio();

        /* Reuse bio in the cache list */
        if (!cache)
            cache = find_unused_bio();

        /* Sleep waiting a bio */
        if (!cache)
            sleep_wait_bio(&bio_cache_head);
    }

    return cache;
}

struct bio * bio_get(uint8_t dev, uint64_t sector)
{
    uint64_t base_sector = BASE_SECTOR(sector);
    struct bio *bio = find_bio(dev, base_sector);

    if (!bio)
    {
        bio = alloc_bio();
        bio->flag = 0;
        bio->dev = dev;
        bio->sector = base_sector;
    }

    while (bio->flag & BIO_FLAG_REFFED)
        sleep_wait_bio(bio);

    split_bio_node(bio);
    insert_into_list_head(bio);

    bio->iter = sector - bio->sector;
    bio->flag |= BIO_FLAG_REFFED;
    return bio;
}

uint64_t bio_last_sector(struct bio *bio)
{
    return bio->sector + SECTORS_PER_BIO - 1;
}

void * bio_data(struct bio *bio)
{
    if (bio->iter < SECTORS_PER_BIO)
    {
        char *buffer = bio->buffer;
        buffer += bio->iter * SECTOR_SIZE;
        return buffer;
    }

    return NULL;
}

void bio_advance_iter(struct bio *bio)
{
    if (bio->iter < SECTORS_PER_BIO)
        bio->iter++;
}

void bio_read(struct bio *bio)
{
    (void)bio;
}

void bio_write(struct bio *bio)
{
    (void)bio;
}

void bio_release(struct bio *bio)
{
    bio->flag &= ~BIO_FLAG_REFFED;

    if (bio->sleep)
    {
        /* Wake up a sleep process waiting on the bio */
        struct process *proc = bio->sleep;
        bio->sleep = proc->sleep;
        proc->sleep = NULL;
        proc->state = PROC_STATE_RUNNING;
    }
    else if (bio_cache_head.sleep)
    {
        /* Wake up a sleep process waiting on the bio list */
        struct process *proc = bio_cache_head.sleep;
        bio_cache_head.sleep = proc->sleep;
        proc->sleep = NULL;
        proc->state = PROC_STATE_RUNNING;
    }
}
