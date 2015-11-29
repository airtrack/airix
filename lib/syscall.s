[bits 32]

%macro syscall 2

global %2
%2:
    mov     eax, %1
    int     0x80
    ret

%endmacro

syscall 0, prints
syscall 1, fork
syscall 2, exit
syscall 3, getpid
syscall 4, open
syscall 5, close
syscall 6, read
syscall 7, write
