#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>

/*
 * Slab memory allocator for alloc kernel object
 * which size is less than PAGE_SIZE.
 */
struct kmem_cache;

/* Create an allocator */
struct kmem_cache * slab_create_kmem_cache(size_t object_size, size_t align);

/* Destroy an allocator */
void slab_destroy_kmem_cache(struct kmem_cache *cache);

/* Alloc an object from allocator */
void * slab_alloc(struct kmem_cache *cache);

/* Free an object into allocator */
void slab_free(struct kmem_cache *cache, void *object);

/* Alloc memory which size is not greater than 1024 */
void * kmalloc(size_t size);

/* Free memory which is allocated by kmalloc */
void kfree(void *ptr);

#endif /* SLAB_H */
