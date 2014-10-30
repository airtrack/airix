[bits 32]
extern cstart

global _start
global display_char

_start:
    call    cstart

display_char:
    push    ebp
    mov     ebp, esp
    mov     ebx, dword [ebp + 8]
    imul    ebx, 80
    add     ebx, dword [ebp + 12]
    imul    ebx, 2
    mov     ah, 0x0F
    mov     al, byte [ebp + 16]
    mov     [gs:ebx], ax
    pop     ebp
    ret
