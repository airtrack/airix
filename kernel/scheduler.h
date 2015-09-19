#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <kernel/process.h>

/* Initialize scheduler */
void sched_initialize();

/* Add process proc into the scheduler, then the scheduler will schedule it. */
void sched_add(struct process *proc);

/* Schedule to scheduler */
void sched();

/* Run the scheduler */
void scheduler();

#endif /* SCHEDULER_H */
