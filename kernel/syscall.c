#include <kernel/base.h>
#include <kernel/klib.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <fs/fs.h>
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

static uint32_t sys_open(va_list ap)
{
    int fd = -1;
    int oflags = 0;
    const char *path = NULL;
    struct file *file = NULL;
    struct process *proc = sched_get_running_proc();

    /* Find a fd. */
    for (int i = 0; i < PROC_MAX_FILE_NUM; ++i)
    {
        if (proc->files[i] == NULL)
        {
            fd = i;
            break;
        }
    }

    if (fd < 0)
        return -1;

    file = vfs_alloc_file();
    if (file == NULL)
        return -1;

    path = va_arg(ap, const char *);
    oflags = va_arg(ap, int);

    if (vfs_open(file, path, oflags) < 0)
    {
        vfs_free_file(file);
        return -1;
    }

    file->f_refs += 1;
    proc->files[fd] = file;
    return fd;
}

static uint32_t sys_close(va_list ap)
{
    int fd = va_arg(ap, int);
    struct process *proc = NULL;

    if (fd < 0 || fd >= PROC_MAX_FILE_NUM)
        return -1;

    proc = sched_get_running_proc();
    if (proc->files[fd] == NULL)
        return -1;

    proc->files[fd]->f_refs -= 1;
    if (proc->files[fd]->f_refs == 0)
    {
        vfs_close(proc->files[fd]);
        vfs_free_file(proc->files[fd]);
    }

    proc->files[fd] = NULL;
    return 0;
}

static uint32_t sys_read(va_list ap)
{
    int fd = va_arg(ap, int);
    char *buffer = va_arg(ap, char *);
    size_t bytes = va_arg(ap, size_t);
    struct process *proc = sched_get_running_proc();

    if (fd < 0 || fd >= PROC_MAX_FILE_NUM)
        return -1;

    if (proc->files[fd] == NULL)
        return -1;

    return vfs_read(proc->files[fd], buffer, bytes);
}

static uint32_t sys_write(va_list ap)
{
    int fd = va_arg(ap, int);
    const char *data = va_arg(ap, const char *);
    size_t bytes = va_arg(ap, size_t);
    struct process *proc = sched_get_running_proc();

    if (fd < 0 || fd >= PROC_MAX_FILE_NUM)
        return -1;

    if (proc->files[fd] == NULL)
        return -1;

    return vfs_write(proc->files[fd], data, bytes);
}

static syscall_t syscalls[] =
{
    sys_prints,
    sys_fork,
    sys_exit,
    sys_getpid,
    sys_open,
    sys_close,
    sys_read,
    sys_write
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
