[bits 32]
extern cstart
extern kernel_main
extern pic_interrupt

global _start
global get_gdtr
global set_gdtr
global set_idtr
global in_byte
global out_byte
global close_int
global start_int
global isr_entry0
global isr_entry7

_start:
    push    ebx
    call    cstart
    mov     esp, 0x10000
    sti
    call    kernel_main

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

close_int:
    cli
    ret

start_int:
    sti
    ret

isr_entry0:
    pushad
    push    0
    call    pic_interrupt
    add     esp, 4
    popad
    iret

isr_entry7:
    pushad
    ; Read master PIC IRR(Interrupt Request Register)
    mov     al, 0x0a
    out     0x20, al
    nop
    nop
    in      al, 0x20
    nop
    nop
    ; Check bit 7
    and     al, 0x80
    ; If it is spurious IRQ, then just ignore it
    jz      .spurious
    push    7
    call    pic_interrupt
    add     esp, 4
.spurious:
    popad
    iret
