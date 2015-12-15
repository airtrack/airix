#include <airix.h>
#include <stdio.h>

int main()
{
    int fd = open("/test", 1);

    if (fd < 0)
    {
        prints("Open file /test error.\n");
    }
    else
    {
        char buf[512];
        int bytes = read(fd, buf, sizeof(buf));

        if (bytes < 0)
        {
            prints("Read file /test fail.\n");
        }
        else
        {
            if (bytes == sizeof(buf))
                bytes -= 1;
            buf[bytes] = '\0';

            prints("File /test content:\n");
            prints(buf);
        }
    }

    for (;;);
    return 0;
}
