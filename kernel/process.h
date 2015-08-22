#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint16_t pid_t;

struct process
{
    pid_t pid;                  /* Process ID */
    void *page_dir;             /* Virtual address space of process */
    uint32_t entry;             /* Entry of process */
    uint32_t kernel_stack;      /* End address of process kernel stack */
    uint32_t kstack_esp;        /* ESP of kernel stack */
    uint32_t user_stack;        /* End address of process user stack */
    struct process *parent;     /* Process's parent */
};

void proc_initialize();
struct process * proc_alloc();
void proc_free(struct process *proc);

bool proc_exec(const char *elf, size_t size);

#endif /* PROCESS_H */
