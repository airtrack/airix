#include <airix.h>
#include <stdio.h>

int main()
{
    pid_t pid = 0;
    int fail = 0;
    char buffer[256] = { 0 };
    prints("Before call fork.\n");

    for (int i = 0; i < 10000; ++i)
    {
        pid = fork();
        if (pid == -1)
        {
            ++fail;
            prints("After call fork, fork fail.\n");
        }
        else if (pid == 0)
        {
            snprintf(buffer, sizeof(buffer),
                     "After call fork, child process, pid: %d.\n", getpid());
            prints(buffer);
            return 0;
        }
        else
        {
            snprintf(buffer, sizeof(buffer),
                     "After call fork, parent process, child pid: %d.\n",
                     pid);
            prints(buffer);
        }
    }

    snprintf(buffer, sizeof(buffer), "Fork completion, fail: %d.\n", fail);
    prints(buffer);
    for (;;);
    return 0;
}
