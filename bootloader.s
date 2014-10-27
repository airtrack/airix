[bits 16]
    org     0x7c00
    mov     ax, cs
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     sp, 0x7c00
    call    load_kernal
    jmp     goto_pm

load_kernal:
    push    bp
    mov     bp, sp
    sub     sp, 4
    mov     ax, 0x1000
    mov     es, ax
    mov     ax, 1
    mov     bx, 0

.reading_sectors:
    mov     word [bp - 4], ax
    mov     word [bp - 2], bx
    mov     cl, 1
    call    read_sector

    mov     ax, word [bp - 4]
    cmp     ax, 128
    jz      .load_success
    inc     ax
    mov     bx, word [bp - 2]
    add     bx, 512
    jmp     .reading_sectors

.load_success:
    add     sp, 4
    pop     bp
    ret

; ax: start sector number
; cl: sector count
; es:bx pointer to buffer
read_sector:
    push    bp
    mov     bp, sp
    sub     sp, 2
    mov     byte [bp - 2], cl
    push    bx

    mov     bl, 18
    div     bl
    inc     ah
    mov     dh, al
    and     dh, 1
    shr     al, 1
    mov     ch, al
    pop     bx
    mov     cl, ah
    mov     dl, 0

.retry_reading:
    mov     ah, 2
    mov     al, byte [bp - 2]
    int     13h
    jc      .retry_reading

    add     sp, 2
    pop     bp
    ret

goto_pm:
    cli
    lgdt    [gdtr]
    ; Open A20
    in      al, 0x92
    or      al, 2
    out     0x92, al
    ; Enable PE
    mov     eax, cr0
    or      al, 1
    mov     cr0, eax
    jmp     0x8:protected_mode

[bits 32]
protected_mode:
    ; Prepare video selector
    mov     ax, 0x18
    mov     gs, ax
    ; Jump into kernal
    jmp     0x8:0x10000

gdt_start:
    ; Unused
    dd      0
    dd      0
    ; Code descriptor
    dw      0xFFFF
    dw      0x0
    db      0x0
    db      0x9a
    db      0xC0
    db      0x0
    ; Data descriptor
    dw      0xFFFF
    dw      0x0
    db      0x0
    db      0x92
    db      0xC0
    db      0x0
    ; Video descriptor
    dw      0xFFFF
    dw      0x8000
    db      0x0B
    db      0x92
    db      0xC0
    db      0x0
gdt_len equ $ - gdt_start

gdtr:
    dw     gdt_len - 1
    dd     gdt_start

    times   510 - ($ - $$) db 0
    dw      0xaa55
