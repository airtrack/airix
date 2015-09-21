#include <kernel/base.h>
#include <kernel/klib.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <stdarg.h>

typedef uint32_t (*syscall_t)(va_list);

static uint32_t sys_prints(va_list ap)
{
    const char *str = va_arg(ap, const char *);
    printk("%s", str);
    return 0;
}

static uint32_t sys_fork(va_list ap)
{
    (void)ap;
    return (uint32_t)sched_fork();
}

static uint32_t sys_exit(va_list ap)
{
    (void)ap;
    proc_exit(sched_get_running_proc(), va_arg(ap, int));
    panic("sys_exit should not return.");
    return 0;
}

static uint32_t sys_getpid(va_list ap)
{
    (void)ap;
    return sched_get_running_proc()->pid;
}

static syscall_t syscalls[] =
{
    sys_prints,
    sys_fork,
    sys_exit,
    sys_getpid
};

void syscall(struct trap_frame *trap)
{
    uint32_t syscall_num = trap->eax;

    if (syscall_num >= ARRAY_SIZE(syscalls))
        return ;

    /*
     * Skip the return address in the user space stack,
     * pointer to the parameters which passed by the user process.
     * Assign the syscall return value to trap->eax which will
     * be returned to the user space.
     */
    trap->eax = syscalls[syscall_num](
        (va_list)(trap->user_esp + sizeof(void *)));
}
