typedef void (*syscall_t)();

static void sys_prints()
{
}

static syscall_t syscalls[] =
{
    sys_prints
};

void syscall()
{
    (void)syscalls;
}
