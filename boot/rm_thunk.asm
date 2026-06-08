; Real-mode INT 13h thunk callable from protected-mode stage2
[BITS 32]

section .text
global rm_read_sector

; int rm_read_sector(uint32_t lba, void *buf, uint8_t drive)
rm_read_sector:
    push ebp
    mov ebp, esp
    pusha
    push es
    push ds

    ; Save return EIP and stack for RM return
    mov [pm_return_eip], dword 0
    mov eax, [ebp + 4]
    mov [rm_lba], eax
    mov eax, [ebp + 8]
    mov [rm_buf], eax
    mov al, byte [ebp + 12]
    mov [rm_drive], al

    ; Enter real mode at rm_entry
    jmp 0x08:do_rm_switch

[BITS 16]
do_rm_switch:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9F00
    mov eax, cr0
    and eax, 0x7FFFFFFE
    mov cr0, eax
    jmp 0x0000:rm_entry

rm_entry:
    mov ax, 0x0000
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9F00

    mov eax, [rm_lba]
    xor edx, edx
    mov ebx, 18
    div ebx
    inc edx
    mov cl, dl
    xor edx, edx
    mov ebx, 2
    div ebx
    mov dh, dl
    mov ch, al

    mov eax, [rm_buf]
    mov bx, ax
    mov ax, 0x0000
    mov es, ax
    shr eax, 4
    mov es, ax
    and bx, 0x000F

    mov ah, 0x02
    mov al, 1
    mov dl, [rm_drive]
    int 0x13
    mov [rm_result], ax

    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:back_pm

[BITS 32]
CODE_SEG equ 0x08
back_pm:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, [pm_esp]

    pop ds
    pop es
    popa
    mov eax, [rm_result]
    and eax, 0xFFFF
    cmp eax, 0
    je .ok
    mov eax, -1
    pop ebp
    ret
.ok:
    xor eax, eax
    pop ebp
    ret

section .bss
align 4
rm_lba:      resd 1
rm_buf:      resd 1
rm_drive:    resb 1
    resb 3
rm_result:   resd 1
pm_return_eip: resd 1
pm_esp:      resd 1
