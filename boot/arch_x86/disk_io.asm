; ATA PIO primary channel disk I/O (protected mode, QEMU -hda / modchip IDE).
[BITS 32]
%include "boot_params.inc"

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global disk_read_sector
global disk_write_sector

%define ATA_DATA   0x1F0
%define ATA_SECCNT 0x1F2
%define ATA_LBA0   0x1F3
%define ATA_LBA1   0x1F4
%define ATA_LBA2   0x1F5
%define ATA_DRIVE  0x1F6
%define ATA_CMD    0x1F7
%define ATA_STATUS 0x1F7

; int disk_read_sector(uint32_t lba, void *buf)
disk_read_sector:
    push ebp
    mov ebp, esp
    push edi
    push esi
    push ebx
    mov ebx, [ebp + 8]
    mov edi, [ebp + 12]
    call ata_wait_ready
    test eax, eax
    jnz .fail
    mov dx, ATA_SECCNT
    mov al, 1
    out dx, al
    mov eax, ebx
    mov dx, ATA_LBA0
    out dx, al
    shr eax, 8
    mov dx, ATA_LBA1
    out dx, al
    shr eax, 8
    mov dx, ATA_LBA2
    out dx, al
    shr eax, 8
    mov dx, ATA_DRIVE
    and al, 0x0F
    or al, 0xE0
    out dx, al
    mov dx, ATA_CMD
    mov al, 0x20
    out dx, al
    call ata_wait_drq
    test eax, eax
    jnz .fail
    mov ecx, 256
    mov dx, ATA_DATA
.rd:
    in ax, dx
    mov [edi], ax
    add edi, 2
    loop .rd
    xor eax, eax
    jmp .done
.fail:
    mov eax, -1
.done:
    pop ebx
    pop esi
    pop edi
    pop ebp
    ret

; int disk_write_sector(uint32_t lba, const void *buf)
disk_write_sector:
    push ebp
    mov ebp, esp
    push edi
    push esi
    push ebx
    mov ebx, [ebp + 8]
    mov esi, [ebp + 12]
    call ata_wait_ready
    test eax, eax
    jnz .fail
    mov dx, ATA_SECCNT
    mov al, 1
    out dx, al
    mov eax, ebx
    mov dx, ATA_LBA0
    out dx, al
    shr eax, 8
    mov dx, ATA_LBA1
    out dx, al
    shr eax, 8
    mov dx, ATA_LBA2
    out dx, al
    shr eax, 8
    mov dx, ATA_DRIVE
    and al, 0x0F
    or al, 0xE0
    out dx, al
    mov dx, ATA_CMD
    mov al, 0x30
    out dx, al
    call ata_wait_drq
    test eax, eax
    jnz .fail
    mov ecx, 256
    mov dx, ATA_DATA
.wr:
    mov ax, [esi]
    add esi, 2
    out dx, ax
    loop .wr
    call ata_wait_ready
    test eax, eax
    jnz .fail
    xor eax, eax
    jmp .done
.fail:
    mov eax, -1
.done:
    pop ebx
    pop esi
    pop edi
    pop ebp
    ret

ata_wait_ready:
    mov ecx, 100000
.ar:
    mov dx, ATA_STATUS
    in al, dx
    test al, 0x80
    jz .ar_ok
    loop .ar
    mov eax, -1
    ret
.ar_ok:
    xor eax, eax
    ret

ata_wait_drq:
    mov ecx, 100000
.ad:
    mov dx, ATA_STATUS
    in al, dx
    test al, 0x08
    jnz .ad_ok
    loop .ad
    mov eax, -1
    ret
.ad_ok:
    xor eax, eax
    ret
