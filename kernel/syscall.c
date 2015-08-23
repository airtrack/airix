#include <kernel/base.h>
#include <kernel/klib.h>
#include <kernel/process.h>
#include <stdarg.h>

typedef void (*syscall_t)(va_list);

static void sys_prints(va_list ap)
{
    const char *str = va_arg(ap, const char *);
    printk("%s", str);
}

static syscall_t syscalls[] =
{
    sys_prints
};

void syscall(struct trap_frame *trap)
{
    uint32_t syscall_num = trap->eax;

    if (syscall_num >= ARRAY_SIZE(syscalls))
        return ;

    /*
     * Skip the return address in user space stack,
     * pointer to parameters which passed by user process.
     */
    syscalls[syscall_num]((va_list)(trap->user_esp + sizeof(void *)));
}
