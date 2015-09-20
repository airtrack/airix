#include <airix.h>
#include <stdio.h>

int main()
{
    pid_t pid = 0;
    prints("Before call fork\n");

    pid = fork();
    if (pid == -1)
    {
        prints("After call fork, fork fail\n");
    }
    else if (pid == 0)
    {
        prints("After call fork, this is child process\n");
    }
    else
    {
        char buffer[256] = { 0 };
        snprintf(buffer, sizeof(buffer),
                 "After call fork, this is parent process, child pid is %d\n",
                 pid);
        prints(buffer);
    }

    for (;;);
    return 0;
}
