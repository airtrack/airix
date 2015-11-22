[bits 32]

global memcpy
global memset
global memcmp

; void * memcpy(void *dst, const void *src, size_t n);
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

; int memcmp(const void *s1, const void *s2, size_t n);
memcmp:
    push    ebp
    mov     ebp, esp
    push    esi
    push    edi
    mov     esi, dword [ebp + 8]
    mov     edi, dword [ebp + 12]
    mov     ecx, dword [ebp + 16]

    xor     eax, eax
    cld
    cmp     ecx, ecx
    repe    cmpsb
    je      .match

    dec     esi
    dec     edi
    xor     ecx, ecx
    mov     al, byte [esi]
    mov     cl, byte [edi]
    sub     eax, ecx

.match:
    pop     edi
    pop     esi
    pop     ebp
    ret
