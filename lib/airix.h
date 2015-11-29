#ifndef AIRIX_H
#define AIRIX_H

#include <stddef.h>

/* System call functions for user process */
void prints(const char *s);

typedef int pid_t;

/*
 * Create a new process, the new process is exact copy of the
 * calling process.
 * Returns a value of 0 to the child process and returns the process ID
 * of the child process to the parent process when success. otherwise, a
 * value of -1 returned to the calling process.
 */
pid_t fork();

/*
 * Terminate the calling process.
 * The value status is returned to the parent as the process's exit status.
 */
void exit(int status);

/* Get the calling process ID. */
pid_t getpid();

/*
 * Open or create a file for reading or writing.
 * If successful, returns a non-negative integer, which is a file descriptor.
 * Returns -1 on failure.
 */
int open(const char *path, int oflag);

/*
 * Close a file descriptor.
 * If successful, returns 0, returns -1 on failure.
 */
int close(int fd);

/*
 * Read data from file.
 * If successful, the number of bytes actually read is returned.
 * Upon reading end-of-file, 0 is returned.
 * Otherwise, -1 is returned.
 */
int read(int fd, void *buf, size_t nbyte);

/*
 * Write data into file.
 * If successful, the number of bytes which were written is returned.
 * Otherwise, -1 is returned.
 */
int write(int fd, const void *buf, size_t nbyte);

#endif /* AIRIX_H */
