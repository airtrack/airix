[bits 32]
extern init_paging
extern kernel_entry
extern pic_interrupt
extern exception_handles

global _start
global get_gdtr
global set_gdtr
global set_idtr
global in_byte
global out_byte
global close_int
global start_int
global halt
global enable_paging

global isr_entry0
global isr_entry7

global divide_by_zero_entry
global debug_entry
global non_maskable_int_entry
global breakpoint_entry
global overflow_entry
global bound_range_exceeded_entry
global invalid_opcode_entry
global device_not_available_entry
global double_fault_entry
global invalid_tss_entry
global segment_not_present_entry
global stack_segment_fault_entry
global general_protection_fault_entry
global page_fault_entry
global fp_exception_entry
global alignment_check_entry
global machine_check_entry
global simd_fp_exception_entry
global virtualization_entry
global security_exception_entry

_start:
    push    ebx
    mov     eax, init_paging
    sub     eax, 0xC0000000
    call    eax
    mov     esp, 0xC0010000
    mov     eax, kernel_entry
    jmp     eax

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

halt:
    hlt

enable_paging:
    mov     eax, dword [esp + 4]
    mov     cr3, eax

    mov     eax, cr0
    or      eax, 0x80000000
    mov     cr0, eax
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
    mov     al, 0x0A
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

%macro exception_entry 2

%2:
    pushad
    call    [exception_handles + %1 * 4]
    popad
    iret

%endmacro

%macro exception_entry_error_code 2

%2:
    pushad
    ; Push error code
    push    dword [esp + 32]
    call    [exception_handles + %1 * 4]
    add     esp, 4
    popad
    ; Skip error code
    add     esp, 4
    iret

%endmacro

; Exceptions entry
exception_entry 0, divide_by_zero_entry
exception_entry 1, debug_entry
exception_entry 2, non_maskable_int_entry
exception_entry 3, breakpoint_entry
exception_entry 4, overflow_entry
exception_entry 5, bound_range_exceeded_entry
exception_entry 6, invalid_opcode_entry
exception_entry 7, device_not_available_entry
exception_entry_error_code 8, double_fault_entry
exception_entry_error_code 10, invalid_tss_entry
exception_entry_error_code 11, segment_not_present_entry
exception_entry_error_code 12, stack_segment_fault_entry
exception_entry_error_code 13, general_protection_fault_entry
exception_entry 16, fp_exception_entry
exception_entry_error_code 17, alignment_check_entry
exception_entry 18, machine_check_entry
exception_entry 19, simd_fp_exception_entry
exception_entry 20, virtualization_entry
exception_entry_error_code 30, security_exception_entry

page_fault_entry:
    pushad
    ; Push error code
    push    dword [esp + 32]
    ; Push the virtual address which caused the page fault
    mov     eax, cr2
    push    eax
    call    [exception_handles + 14 * 4]
    add     esp, 8
    popad
    ; Skip error code
    add     esp, 4
    iret
