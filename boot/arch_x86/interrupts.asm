; CPU interrupt and power-management helpers.
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global disable_interrupts_asm
global enable_interrupts_asm
global cpu_pause
global system_reboot

disable_interrupts_asm:
    cli
    ret

enable_interrupts_asm:
    sti
    ret

cpu_pause:
    pause
    ret

; Warm reset via QEMU/PC chipset reset register.
system_reboot:
    mov dx, 0xCF9
    mov al, 0x06
    out dx, al
.hang:
    hlt
    jmp .hang
