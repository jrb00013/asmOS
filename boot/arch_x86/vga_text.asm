; VGA text mode output at 0xB8000 (protected mode).
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global print_string
global vga_putchar

extern cursor_row
extern cursor_col

%define VGA_BASE 0xB8000
%define VGA_W    80
%define VGA_H    25
%define VGA_ATTR 0x07

; void vga_putchar(char c)
vga_putchar:
    push ebx
    push esi
    push edi
    mov al, [esp + 16]
    cmp al, 10
    je .newline
    cmp al, 8
    je .backspace
    mov ebx, [cursor_row]
    mov ecx, [cursor_col]
    imul ebx, VGA_W
    add ebx, ecx
    shl ebx, 1
    mov edi, VGA_BASE
    add edi, ebx
    mov ah, VGA_ATTR
    mov [edi], ax
    inc dword [cursor_col]
    mov eax, [cursor_col]
    cmp eax, VGA_W
    jl .scroll
    mov dword [cursor_col], 0
    inc dword [cursor_row]
    jmp .scroll
.newline:
    inc dword [cursor_row]
    mov dword [cursor_col], 0
    jmp .scroll
.backspace:
    cmp dword [cursor_col], 0
    je .done
    dec dword [cursor_col]
    mov ebx, [cursor_row]
    mov ecx, [cursor_col]
    imul ebx, VGA_W
    add ebx, ecx
    shl ebx, 1
    mov edi, VGA_BASE
    add edi, ebx
    mov ax, 0x0720
    mov [edi], ax
    jmp .done
.scroll:
    mov eax, [cursor_row]
    cmp eax, VGA_H
    jl .done
    mov esi, VGA_BASE + (VGA_W * 2)
    mov edi, VGA_BASE
    mov ecx, (VGA_W * (VGA_H - 1))
    rep movsw
    mov edi, VGA_BASE + ((VGA_H - 1) * VGA_W * 2)
    mov ecx, VGA_W
    mov ax, 0x0720
    rep stosw
    mov dword [cursor_row], VGA_H - 1
.done:
    pop edi
    pop esi
    pop ebx
    ret

; void print_string(const char *s) — esi on entry for legacy, also cdecl
print_string:
    push esi
    mov esi, [esp + 8]
.ps_loop:
    lodsb
    test al, al
    jz .ps_done
    movzx eax, al
    push eax
    call vga_putchar
    add esp, 4
    jmp .ps_loop
.ps_done:
    pop esi
    ret
