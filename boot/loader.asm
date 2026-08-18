; ASMOS stage2 loader at 0x7E00 — FAT12 KERNEL.BIN + protected mode
[BITS 16]
[ORG 0x7E00]

%define FAT_DATA 65
%define FAT_ROOT 51
%define FAT_START 33
%define FAT_SEG 0x1000
%define ROOT_SEG 0x1100
; The boot sector's BPB "DriveNumber" field is at offset 35 from 0x7C00
; (right after the 4-byte TotalSectors32 field), i.e. absolute 0x7C23 —
; NOT 0x7C24 (that's the following "Reserved1" byte, always 0). Confirmed
; by disassembling the actual assembled boot sector's
; "mov [DriveNumber], dl" instruction.
%define BPB_DRIVE 0x7C23

; bootsect.asm loads this file to a fixed physical address (0000:7E00) and
; jumps straight there — it has no idea where a "loader_entry" label ends
; up inside the assembled file. That means whatever is assembled at
; ORG 0x7E00 (this file's very first byte) *is* the real entry point.
; %include "debugcon.asm" used to sit here, before the loader_entry label,
; so debug_putc/debug_puts (not loader_entry) occupied 0x7E00, and
; bootsect.asm's jump landed inside debug_putc mid-function with no
; return address on the stack for its `ret` — an effectively random jump.
; Jump to loader_entry explicitly, first, so it doesn't matter what else
; gets included below.
jmp loader_entry

%include "debugcon.asm"

loader_entry:
    call enable_a20
    mov dl, [BPB_DRIVE]
    mov si, boot_msg
    call print_string
    mov si, dbg_loader
    call debug_puts
    call fatload_kernel
    mov si, dbg_loaded
    call debug_puts
    mov si, ok_msg
    call print_string
    call switch_to_pm

; fatload_kernel's cluster-loading loop (below) targets ES:BX = 0xFFFF:0x0010,
; i.e. the physical 1MB mark, directly via real-mode INT13h — a classic
; "load high" trick. That only works if the A20 line is enabled; otherwise
; any linear address >= 1MB wraps back down near address 0, and (observed
; empirically under QEMU/SeaBIOS) the INT13h read can also just fail
; outright with AH=0x20 ("controller failure") instead of silently
; wrapping. Enable the fast A20 gate (port 0x92, bit 1) before any of
; that happens — this was previously missing entirely.
enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

kernel_name db 'KERNEL  BIN'

fatload_kernel:
    pusha
    mov ax, FAT_SEG
    mov es, ax
    xor bx, bx
    mov eax, FAT_START
    mov ecx, 9
    call read_lba
    mov ax, ROOT_SEG
    mov es, ax
    xor bx, bx
    mov eax, FAT_ROOT
    mov ecx, 14
    call read_lba

    mov ax, ROOT_SEG
    mov ds, ax
    xor di, di
    mov cx, 224
.find:
    push cx
    push di
    mov si, kernel_name
    mov cx, 11
    ; cmpsb compares [DS:SI] against [ES:DI]. DS is ROOT_SEG here (needed
    ; right below for "mov ax,[di+26]"), but kernel_name is a label in
    ; THIS file, loaded at CS=0 — under DS=ROOT_SEG, [DS:SI] pointed at
    ; unrelated/uninitialized memory in the root-directory segment instead
    ; of the actual "KERNEL  BIN" string, so this loop was comparing
    ; garbage against every directory entry and "matching" whichever
    ; empty/deleted entry happened to look like that garbage — not the
    ; real KERNEL.BIN entry. Override the source segment to CS so SI
    ; actually reads kernel_name.
    repe cs cmpsb
    pop di
    pop cx
    jz .got
    add di, 32
    loop .find
    jmp disk_err
.got:
    mov ax, [di + 26]
    mov [cs:cluster], ax

    ; Stage the kernel into low conventional memory (segment 0x2000 ==
    ; physical 0x20000) instead of reading straight to the 1MB mark via
    ; ES:BX = 0xFFFF:0x0010. That "load high" trick needs a BIOS whose
    ; INT13h implementation tolerates a >=1MB linear destination, and
    ; empirically SeaBIOS here returns AH=0x20 ("controller failure") for
    ; it even with A20 enabled. Loading low is what every mainstream
    ; bootloader does instead; switch_to_pm's 32-bit copy (below) then
    ; relocates the staged bytes up to 0x100000 once we're in protected
    ; mode and can address memory linearly without segment tricks.
    mov ax, 0x2000
    mov es, ax
    xor bx, bx

    mov ax, FAT_SEG
    mov ds, ax

.cloop:
    mov ax, [cs:cluster]
    cmp ax, 0xFF8
    jae .done

    mov si, ax
    mov cx, si
    dec cx
    dec cx
    mov ax, FAT_DATA
    add ax, cx
    movzx eax, ax
    call read_es
    add word [cs:kernel_size], 512
    adc word [cs:kernel_size + 2], 0

    add bx, 512
    jnc .nc
    push ax
    mov ax, es
    add ax, 0x1000
    mov es, ax
    pop ax
.nc:
    mov ax, [cs:cluster]
    mov si, ax
    shr si, 1
    add si, ax
    mov ax, [si]
    test word [cs:cluster], 1
    jz .nx
    shr ax, 4
    jmp .st
.nx:
    and ax, 0x0FFF
.st:
    mov [cs:cluster], ax
    jmp .cloop
.done:
    xor ax, ax
    mov ds, ax
    popa
    ret

read_lba:
.read_loop:
    push eax
    push ecx
    push bx
    call lba_chs
    mov ah, 0x02
    mov al, 1
    mov dl, [BPB_DRIVE]
    int 0x13
    jc disk_err
    pop bx
    add bx, 512
    pop ecx
    pop eax
    inc eax
    loop .read_loop
    ret

read_es:
    ; eax = LBA, es:bx = buffer
    push eax
    call lba_chs
    mov ah, 0x02
    mov al, 1
    mov dl, [BPB_DRIVE]
    int 0x13
    jc disk_err
    pop eax
    ret

lba_chs:
    ; eax = LBA in, cl/dh/ch out; preserves bx
    push bx
    xor edx, edx
    mov bx, 18
    div bx
    inc dx
    mov cl, dl
    xor edx, edx
    mov bx, 2
    div bx
    mov dh, dl
    mov ch, al
    pop bx
    ret

align 8
gdt:
    dq 0
    dw 0xFFFF, 0, 0x9A00, 0x00CF
    dw 0xFFFF, 0, 0x9200, 0x00CF
gdt_end:
gdtr:
    dw gdt_end - gdt - 1
    dd gdt
CODE_SEG equ 0x08
DATA_SEG equ 0x10

switch_to_pm:
    cli
    lgdt [gdtr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp CODE_SEG:pm32

[BITS 32]
pm32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x0009F000

    ; The kernel was staged at physical 0x20000 (real mode couldn't read
    ; it straight to 0x100000 — see fatload_kernel's comment). Now that
    ; we're in protected mode with flat 32-bit addressing, relocate it up
    ; to where it's actually meant to run, using the byte count tracked
    ; in kernel_size while it was being read from disk.
    mov esi, 0x20000
    mov edi, 0x100000
    mov ecx, [kernel_size]
    add ecx, 3
    shr ecx, 2          ; round up to whole dwords
    rep movsd

    ; The kernel image is copied to 0x100000 (matching llinker/linker.ld's
    ; ORIGIN), but that address holds the 4-byte .bootsig magic + 4K
    ; alignment padding, not code — _kernel_start (src/kernel_start.c) is
    ; deliberately pinned to 0x101000 (see the ".text.boot" section in
    ; linker.ld) specifically so this jump target can be a fixed constant
    ; instead of needing an ELF symbol table lookup that isn't available
    ; here. Jumping to 0x100000 itself would execute the ASMK magic bytes
    ; as an instruction — this was landing on whatever code the linker's
    ; default object order happened to place there instead.
    jmp CODE_SEG:0x101000

[BITS 16]
disk_err:
    mov si, dbg_diskerr
    call debug_puts
    mov si, err_msg
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz .x
    mov ah, 0x0E
    int 0x10
    jmp print_string
.x:
    ret

boot_msg db 'ASMOS loader', 13, 10, 0
ok_msg db 'PM...', 13, 10, 0
err_msg db 'ERR', 13, 10, 0
dbg_loader db 'DEBUG:LOADER_START', 10, 0
dbg_loaded db 'DEBUG:KERNEL_LOADED', 10, 0
dbg_diskerr db 'DEBUG:DISK_ERR', 10, 0
cluster dw 0
kernel_size dd 0
