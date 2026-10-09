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
extern floppy_read_sector
extern floppy_write_sector

; The volume this kernel boots lives on the floppy controller (-fda), not
; on the ATA primary channel, so sector I/O goes through the FDC driver by
; default. Define PLATFORM_X86_DISK_ATA to route back to the ATA PIO driver
; on hardware that actually has a disk on the primary channel.
%ifdef PLATFORM_X86_DISK_ATA
fat12_read_sector:
    jmp disk_read_sector

fat12_write_sector:
    jmp disk_write_sector
%else
fat12_read_sector:
    jmp floppy_read_sector

fat12_write_sector:
    jmp floppy_write_sector
%endif
