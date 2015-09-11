#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <kernel/process.h>

void sched_initialize();

/*
 * Add process proc into the scheduler,
 * then the scheduler will schedule it.
 */
void sched_add(struct process *proc);

/* Schedule running process */
void sched_process(struct process *proc);

/* Schedule one process */
void sched();

#endif /* SCHEDULER_H */
