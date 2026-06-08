; Switch from real mode to protected mode and jump to stage2
[BITS 16]

%include "gdt.asm"

extern stage2_pm_entry

switch_to_pm:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:init_pm32

[BITS 32]
init_pm32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x0009F000
    call stage2_pm_entry
    jmp $
