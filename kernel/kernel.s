[bits 32]
extern init_paging
extern kernel_entry
extern pic_interrupt
extern exception_handles
extern syscall

global _start
global set_gdtr
global set_idtr
global set_cr3
global set_tss
global in_byte
global in_dword
global insw
global out_byte
global out_dword
global close_int
global start_int
global halt
global switch_kcontext
global ret_user_space
global syscall_entry
global isr_entry0
global isr_entry1
global isr_entry2
global isr_entry3
global isr_entry4
global isr_entry5
global isr_entry6
global isr_entry7
global isr_entry8
global isr_entry9
global isr_entry10
global isr_entry11
global isr_entry12
global isr_entry13
global isr_entry14
global isr_entry15

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
    mov     ebp, esp
    mov     eax, kernel_entry
    jmp     eax

set_gdtr:
    mov     eax, dword [esp + 4]
    lgdt    [eax]
    ret

set_idtr:
    mov     eax, dword [esp + 4]
    lidt    [eax]
    ret

set_cr3:
    mov     eax, dword [esp + 4]
    mov     cr3, eax

    mov     eax, cr0
    or      eax, 0x80000000
    mov     cr0, eax
    ret

set_tss:
    mov     ax, word [esp + 4]
    ltr     ax
    ret

in_byte:
    mov     edx, dword [esp + 4]
    xor     eax, eax
    in      al, dx
    nop
    nop
    ret

in_dword:
    mov     edx, dword [esp + 4]
    xor     eax, eax
    in      eax, dx
    nop
    nop
    ret

insw:
    push    edi
    mov     dx, word [esp + 8]
    mov     ecx, dword [esp + 12]
    mov     edi, dword [esp + 16]
    rep     insw
    pop     edi
    ret

out_byte:
    mov     edx, dword [esp + 4]
    mov     al, byte [esp + 8]
    out     dx, al
    nop
    nop
    ret

out_dword:
    mov     edx, dword [esp + 4]
    mov     eax, dword [esp + 8]
    out     dx, eax
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

; Switch kernel stack, prototype in c:
;     void switch_kcontext(struct kstack_context **cur,
;                          struct kstack_context *new);
switch_kcontext:
    mov     ecx, dword [esp + 4]    ; cur
    mov     eax, dword [esp + 8]    ; new

    ; Save registers of struct kstack_context
    push    edi
    push    esi
    push    ebp
    push    ebx

    ; Switch stack
    mov     [ecx], esp              ; Save esp to *cur
    mov     esp, eax                ; Restore esp from new

    ; Restore registers of struct kstack_context
    pop     ebx
    pop     ebp
    pop     esi
    pop     edi

    ret

%macro int_enter 0

    pushad
    push    ds
    push    es
    push    fs
    push    gs

    ; 0x10 is kernel code descriptor selector
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

%endmacro

%macro int_ret 0

    pop     gs
    pop     fs
    pop     es
    pop     ds
    popad

    ; Skip error code
    add     esp, 4
    iret

%endmacro

ret_user_space:
    mov     esp, dword [esp + 4]
    int_ret

syscall_entry:
    push    0
    int_enter

    push    esp
    call    syscall
    add     esp, 4

    int_ret

%macro isr_entry 1

isr_entry%1:
    push    0
    int_enter

    push    %1
    call    pic_interrupt
    add     esp, 4

    int_ret

%endmacro

isr_entry 0
isr_entry 1
isr_entry 2
isr_entry 3
isr_entry 4
isr_entry 5
isr_entry 6

isr_entry 8
isr_entry 9
isr_entry 10
isr_entry 11
isr_entry 12
isr_entry 13
isr_entry 14
isr_entry 15

isr_entry7:
    push    0
    int_enter

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
    int_ret

%macro excep 2

%2:
    push    0
    int_enter

    call    [exception_handles + %1 * 4]

    int_ret

%endmacro

%macro excep_error_code 2

%2:
    int_enter

    ; Push error code
    push    dword [esp + 48]
    call    [exception_handles + %1 * 4]
    add     esp, 4

    int_ret

%endmacro

; Exceptions entry
excep 0, divide_by_zero_entry
excep 1, debug_entry
excep 2, non_maskable_int_entry
excep 3, breakpoint_entry
excep 4, overflow_entry
excep 5, bound_range_exceeded_entry
excep 6, invalid_opcode_entry
excep 7, device_not_available_entry
excep 16, fp_exception_entry
excep 18, machine_check_entry
excep 19, simd_fp_exception_entry
excep 20, virtualization_entry
excep_error_code 8, double_fault_entry
excep_error_code 10, invalid_tss_entry
excep_error_code 11, segment_not_present_entry
excep_error_code 12, stack_segment_fault_entry
excep_error_code 13, general_protection_fault_entry
excep_error_code 17, alignment_check_entry
excep_error_code 30, security_exception_entry

page_fault_entry:
    int_enter

    ; Push error code
    push    dword [esp + 48]
    ; Push the virtual address which caused the page fault
    mov     eax, cr2
    push    eax
    call    [exception_handles + 14 * 4]
    add     esp, 8

    int_ret
