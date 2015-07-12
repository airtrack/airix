[bits 32]

global memcpy
global memset

; void * memcpy(void *dst, const void *src, size_t nJ);
memcpy:
    push    ebp
    mov     ebp, esp
    push    edi
    push    esi
    mov     edi, dword [ebp + 8]
    mov     esi, dword [ebp + 12]
    mov     ecx, dword [ebp + 16]
    cld
    rep     movsb
    mov     eax, dword [ebp + 8]
    pop     esi
    pop     edi
    pop     ebp
    ret

; void * memset(void *b, int c, size_t len);
memset:
    push    ebp
    mov     ebp, esp
    push    edi
    mov     edi, dword [ebp + 8]
    mov     eax, dword [ebp + 12]
    mov     ecx, dword [ebp + 16]
    cld
    rep     stosb
    mov     eax, dword [ebp + 8]
    pop     edi
    pop     ebp
    ret
