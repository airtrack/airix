[bits 32]
extern cstart

global _start
global get_gdtr
global set_gdtr
global display_char

_start:
    call    cstart

get_gdtr:
    push    ebp
    mov     ebp, esp
    mov     eax, dword [ebp + 8]
    sgdt    [eax]
    pop     ebp
    ret

set_gdtr:
    push    ebp
    mov     ebp, esp
    mov     eax, dword [ebp + 8]
    lgdt    [eax]
    pop     ebp
    ret

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
