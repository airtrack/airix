#include <airix.h>

int main()
{
    prints("We are in user process call syscall.\n");
    prints("We are in user process call syscall again.\n");
    prints("We are in user process call syscall again again.\n");

    for (;;);
    return 0;
}
