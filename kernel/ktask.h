#ifndef KTASK_H
#define KTASK_H

/* Kernel task function data struct */
struct kernel_task
{
    void (*task_func)(void *);  /* Task function */
    void *data;                 /* Task function parameter */
    struct kernel_task *prev;
    struct kernel_task *next;
};

void ktask_initialize();

/*
 * Register/unregister kernel task function.
 * Hold kernel_task struct when it register, release until unregister.
 */
void ktask_register(struct kernel_task *task);
void ktask_unregister(struct kernel_task *task);

#endif /* KTASK_H */
