#ifndef BIO_H
#define BIO_H

#include <stdbool.h>
#include <stdint.h>

struct bio;

/* Initialize block IO */
void bio_initialize();

/* Get block IO, after some operations, call bio_release to release */
struct bio * bio_get(uint8_t dev, uint64_t sector);

/* Get the last sector number of the bio */
uint64_t bio_last_sector(struct bio *bio);

/*
 * Returns current sector buffer if the iterator pointer a valid sector,
 * otherwise, returns NULL.
 */
void * bio_data(struct bio *bio);

/*
 * Advance the iterator of the bio, then call bio_data to get next sector
 * buffer. Do nothing if the iterator of the bio is at the end of the bio.
 */
void bio_advance_iter(struct bio *bio);

/* Read sectors from block device */
bool bio_read(struct bio *bio);

/* Write sector buffer content to block device */
void bio_write(struct bio *bio);

/* Release the bio */
void bio_release(struct bio *bio);

#endif /* BIO_H */
