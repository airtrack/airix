#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <kernel/process.h>

/* Initialize scheduler */
void sched_initialize();

/* Add the process proc into the scheduler,
 * then the scheduler will schedule it. */
void sched_add(struct process *proc);

/* Remove the process proc from the scheduler */
void sched_remove(struct process *proc);

/* Schedule to scheduler */
void sched();

/* Syscall fork */
pid_t sched_fork();

/* Get the running process */
struct process * sched_get_running_proc();

/* Run the scheduler */
void scheduler();

#endif /* SCHEDULER_H */
