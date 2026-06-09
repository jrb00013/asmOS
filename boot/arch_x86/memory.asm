; Memory size from boot-time E820 probe (real mode writes BOOT_PARAMS_PHYS).
[BITS 32]
%include "boot_params.inc"

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global get_memory_info
global detect_ps2_memory

; uint32_t detect_ps2_memory(void) — returns megabytes
detect_ps2_memory:
    mov eax, [BOOT_PARAMS_PHYS + BOOT_PARAM_MEM_MB]
    test eax, eax
    jnz .done
    mov eax, 32
.done:
    ret

; uint32_t get_memory_info(void) — returns kilobytes (shell meminfo)
get_memory_info:
    call detect_ps2_memory
    shl eax, 10
    ret
