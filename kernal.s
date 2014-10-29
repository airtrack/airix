[bits 32]
global _start

_start:
    mov     ah, 0x0F
    mov     al, 'K'
    mov     [gs:((80 * 20 + 39) * 2)], ax
    jmp     $
