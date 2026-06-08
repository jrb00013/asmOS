; GDT for protected mode switch
[BITS 16]

section .data
align 8
gdt_start:
    dq 0
    dw 0xFFFF, 0x0000, 0x9A00, 0x00CF   ; code 0x08
    dw 0xFFFF, 0x0000, 0x9200, 0x00CF   ; data 0x10
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ 0x08
DATA_SEG equ 0x10
