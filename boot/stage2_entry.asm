[BITS 32]
section .text
global stage2_pm_entry
extern _stage2_start

stage2_pm_entry:
    jmp _stage2_start
