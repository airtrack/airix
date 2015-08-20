#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef uint16_t pid_t;

struct process
{
    pid_t pid;                  /* Process ID */
    void *page_dir;             /* Virtual address space of process */
    void *entry;                /* Entry of process */
    struct process *parent;     /* Process's parent */
};

void proc_initialize();
struct process * proc_alloc();
void proc_free(struct process *proc);

#endif /* PROCESS_H */
