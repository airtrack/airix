[bits 32]
extern cstart

global _start
global get_gdtr
global set_gdtr
global set_idtr
global in_byte
global out_byte
global display_char

_start:
    call    cstart

get_gdtr:
    mov     eax, dword [esp + 4]
    sgdt    [eax]
    ret

set_gdtr:
    mov     eax, dword [esp + 4]
    lgdt    [eax]
    ret

set_idtr:
    mov     eax, dword [esp + 4]
    lidt    [eax]
    ret

in_byte:
    mov     edx, dword [esp + 4]
    xor     eax, eax
    in      al, dx
    nop
    nop
    ret

out_byte:
    mov     edx, dword [esp + 4]
    mov     al, byte [esp + 8]
    out     dx, al
    nop
    nop
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
