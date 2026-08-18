; Cooperative task context switch (x86 only).
[BITS 32]
section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global task_yield_asm
global run_scheduler_asm

; void task_yield_asm(uint32_t *save_esp, uint32_t load_esp)
;
; Full cooperative context switch: saves EFLAGS + all GP registers of the
; calling (outgoing) task onto its own stack, records the resulting esp,
; then switches onto the incoming task's stack and restores its EFLAGS +
; GP registers before returning into it. This is symmetric with the frame
; built by add_task() in scheduler.c, so a task that has never run yet and
; a task that is being resumed after a prior yield are restored identically.
task_yield_asm:
    pushfd                   ; save caller's EFLAGS (IF, etc.)
    pusha                    ; save caller's GP registers (32 bytes)
    mov eax, [esp + 36]      ; save_esp arg (pusha=32 + pushfd=4 bytes above it)
    mov [eax], esp           ; record outgoing task's stack pointer
    mov esp, [esp + 40]      ; load_esp arg -> switch onto incoming task's stack
    popa                     ; restore incoming task's GP registers
    popfd                    ; restore incoming task's EFLAGS
    ret                      ; return into incoming task (or its entry point)

; void run_scheduler_asm(uint32_t load_esp)
run_scheduler_asm:
    mov esp, [esp + 4]       ; load_esp arg
    popa                     ; restore task 0's GP registers
    popfd                    ; restore task 0's EFLAGS
    ret                      ; jump into task 0's entry point
