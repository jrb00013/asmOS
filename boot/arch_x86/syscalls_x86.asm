; x86 protected-mode syscalls — real implementations, no PS2 MMIO stubs.
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global sys_read_line
global sys_exit
global sys_network_init
global sys_network_send
global sys_network_receive
global sys_graphics_init
global exit_program

extern keyboard_poll_scancode
extern scancode_to_ascii
extern vga_putchar
extern print_string
extern system_reboot

extern plat_net_init
extern plat_net_send
extern plat_net_recv
extern video_set_mode_text

section .bss
align 4
shift_down resd 1

section .rodata
exit_msg db "ASMOS shutting down.", 13, 10, 0

section .text

sys_network_init:
    jmp plat_net_init

; int plat_net_send(const void *data, size_t len)
sys_network_send:
    push ebp
    mov ebp, esp
    push dword [ebp + 12]
    push dword [ebp + 8]
    call plat_net_send
    add esp, 8
    pop ebp
    ret

; int plat_net_recv(void *buf, size_t max_len)
sys_network_receive:
    push ebp
    mov ebp, esp
    push dword [ebp + 12]
    push dword [ebp + 8]
    call plat_net_recv
    add esp, 8
    pop ebp
    ret

sys_graphics_init:
    jmp video_set_mode_text

sys_read_line:
    push ebp
    mov ebp, esp
    push edi
    push esi
    push ebx
    mov edi, [ebp + 8]
    mov ebx, [ebp + 12]
    dec ebx
    xor esi, esi
.rl_loop:
    call keyboard_poll_scancode
    test eax, eax
    jz .rl_loop
    mov ecx, eax
    cmp cl, 0x2A
    je .sh_on
    cmp cl, 0x36
    je .sh_on
    cmp cl, 0xAA
    je .sh_off
    cmp cl, 0xB6
    je .sh_off
    cmp cl, 0x1C
    je .rl_done
    cmp cl, 0x0E
    je .rl_bs
    push dword [shift_down]
    push ecx
    call scancode_to_ascii
    add esp, 8
    test al, al
    jz .rl_loop
    cmp esi, ebx
    jae .rl_loop
    mov [edi + esi], al
    inc esi
    movzx eax, al
    push eax
    call vga_putchar
    add esp, 4
    jmp .rl_loop
.sh_on:
    mov dword [shift_down], 1
    jmp .rl_loop
.sh_off:
    mov dword [shift_down], 0
    jmp .rl_loop
.rl_bs:
    test esi, esi
    jz .rl_loop
    dec esi
    push 8
    call vga_putchar
    add esp, 4
    push ' '
    call vga_putchar
    add esp, 4
    push 8
    call vga_putchar
    add esp, 4
    jmp .rl_loop
.rl_done:
    mov byte [edi + esi], 0
    push 10
    call vga_putchar
    add esp, 4
    mov eax, edi
    pop ebx
    pop esi
    pop edi
    pop ebp
    ret

sys_exit:
    mov esi, exit_msg
    call print_string
    cli
    call system_reboot

exit_program:
    xor ebx, ebx
    jmp sys_exit
