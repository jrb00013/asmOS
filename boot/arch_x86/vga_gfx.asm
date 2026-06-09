; Mode 13h framebuffer rectangle fill (0xA0000).
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global sys_graphics_draw

%define FB_ADDR 0xA0000
%define FB_WIDTH 320

; sys_graphics_draw(x, y, color, width, height) — register args (legacy)
sys_graphics_draw:
    push ebp
    mov ebp, esp
    push edi
    push esi
    push ebx
    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]
    mov edx, [ebp + 20]
    mov esi, [ebp + 24]
    test edx, edx
    jz .done
    test esi, esi
    jz .done
.row:
    push esi
    push edx
    push ebx
    push eax
    imul eax, FB_WIDTH
    add eax, ebx
    add eax, FB_ADDR
    mov edi, eax
    mov al, cl
    mov ecx, edx
    rep stosb
    pop eax
    pop ebx
    pop edx
    pop esi
    inc ebx
    dec esi
    jnz .row
.done:
    pop ebx
    pop esi
    pop edi
    pop ebp
    ret
