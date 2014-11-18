[bits 32]

global memcpy

; void * memcpy(void *dst, const void *src, size_t nJ);
memcpy:
    push    ebp
    mov     ebp, esp
    mov     edi, dword [ebp + 8]
    mov     esi, dword [ebp + 12]
    mov     ecx, dword [ebp + 16]
    cld
    rep     movsb
    mov     eax, dword [ebp + 8]
    pop     ebp
    ret
