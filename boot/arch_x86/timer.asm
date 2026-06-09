; PIT channel 0 + TSC fallback for timing syscalls.
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .bss
align 4
pit_latch resd 1

section .text

global sys_timer_init
global sys_timer_get

; Program PIT channel 0 to ~100 Hz and latch initial count.
sys_timer_init:
    mov al, 0x34
    out 0x43, al
    mov ax, 11932
    out 0x40, al
    mov al, ah
    out 0x40, al
    mov dword [pit_latch], 0
    ret

; Return milliseconds since init using TSC (real time, no stub counter).
sys_timer_get:
    rdtsc
    mov ecx, 100000
    xor edx, edx
    div ecx
    ret
