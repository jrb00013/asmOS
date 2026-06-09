; Cooperative task context switch (x86 only).
[BITS 32]
section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global task_yield_asm
global run_scheduler_asm

; void task_yield_asm(uint32_t *save_esp, uint32_t load_esp)
task_yield_asm:
    pusha
    mov eax, [esp + 36]     ; save_esp (pusha adds 32 bytes)
    mov [eax], esp
    mov esp, [esp + 40]     ; load_esp
    popa
    ret

; void run_scheduler_asm(uint32_t load_esp)
run_scheduler_asm:
    mov esp, [esp + 4]
    popa
    ret
