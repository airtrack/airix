#include <kernel/klib.h>
#include <lib/stdio.h>

static const char *desc = "An Internal error was detected.\
 The system has been halted.\nPlease restart computer.\n";

void panic(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    close_int();
    clear_screen();
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printk("%s\n\n%s", buf, desc);

    halt();
}
