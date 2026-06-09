[BITS 32]
section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global init_fat12
global fat12_read_sector
global fat12_write_sector
global init_ps2_controllers

section .data
sectors_per_track dd 18
heads_per_cylinder dd 2

section .text

; x86 keyboard controller init (PS/2 ports)
init_ps2_controllers:
    pusha
    mov al, 0xAD
    out 0x64, al
    mov al, 0xAE
    out 0x64, al
    mov al, 0xA7
    out 0x64, al
    mov al, 0xA8
    out 0x64, al
    popa
    ret

; Tables are loaded by platform/x86/hal_storage.c via disk_read_sector.
init_fat12:
    ret

extern disk_read_sector
extern disk_write_sector

fat12_read_sector:
    jmp disk_read_sector

fat12_write_sector:
    jmp disk_write_sector
