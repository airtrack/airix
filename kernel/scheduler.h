#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <kernel/process.h>

void sched_initialize();

/* Schedule running process */
void sched_process(struct process *proc);

#endif /* SCHEDULER_H */
