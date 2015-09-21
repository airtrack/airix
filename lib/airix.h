#ifndef AIRIX_H
#define AIRIX_H

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

#endif /* AIRIX_H */
