; PC speaker (PIT channel 2) for x86 sound syscalls.
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global sys_sound_init
global sys_sound_play

sys_sound_init:
    in al, 0x61
    or al, 1
    out 0x61, al
    ret

; sys_sound_play(void *data, uint32_t len, uint32_t freq)
; cdecl stack: data, len, freq — uses freq in ecx per legacy convention from shell
sys_sound_play:
    push ebp
    mov ebp, esp
    mov ecx, [ebp + 16]
    test ecx, ecx
    jz .off
    mov eax, 1193182
    xor edx, edx
    div ecx
    mov al, 0xB6
    out 0x43, al
    out 0x42, al
    xchg ah, al
    out 0x42, al
    in al, 0x61
    or al, 3
    out 0x61, al
    ret
.off:
    in al, 0x61
    and al, 0xFC
    out 0x61, al
    pop ebp
    ret
