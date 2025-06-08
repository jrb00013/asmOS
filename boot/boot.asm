; Enhanced PS2-compatible x86 bootloader
[bits 16]
[org 0x7C00]

; Bootloader entry point
start:
    cli                     ; Disable interrupts during setup
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; Stack grows down from bootloader
    sti                     ; Re-enable interrupts
    mov [boot_drive], dl    ; Store boot drive number

    ; Clear screen
    mov ax, 0x0003          ; 80x25 text mode
    int 0x10

    ; Print boot message
    mov si, boot_msg
    call print_string

    ; Load kernel from disk
    call load_kernel

    ; Check if kernel loaded successfully
    cmp word [kernel_size], 0
    je load_error

    ; Jump to kernel
    mov si, kernel_loaded_msg
    call print_string
    
    ; Prepare to jump to kernel
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Far jump to kernel entry point
    jmp 0x1000:0x0000

load_kernel:
    mov ah, 0x02            ; BIOS read sectors function
    mov al, 15              ; Number of sectors to read
    mov ch, 0x00            ; Cylinder 0
    mov dh, 0x00            ; Head 0
    mov cl, 0x02            ; Sector 2 (1-based)
    mov bx, 0x0000          ; ES:BX buffer (0x1000:0x0000)
    mov es, bx              ; ES = 0 (will be 0x1000 later)
    mov bx, 0x1000          ; Load at 0x1000:0000 (physical 0x10000)
    mov dl, [boot_drive]    ; Drive number
    
    ; Set ES:BX properly for loading
    push 0x1000
    pop es
    xor bx, bx
    
    int 0x13                ; BIOS disk read
    jc disk_error           ; Jump if error (carry set)
    
    ; Store kernel size (assuming it's in the first word)
    mov word [kernel_size], 15 * 512  ; 15 sectors * 512 bytes
    
    ret

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $

load_error:
    mov si, load_error_msg
    call print_string
    jmp $

print_string:
    lodsb                   ; Load next character
    or al, al               ; Test for null terminator
    jz done
    mov ah, 0x0E            ; BIOS teletype function
    int 0x10
    jmp print_string

parse_elf:
    ; Check ELF magic number
    mov si, bx              ; SI = offset
    cmp word [es:si], 0x457F    ; 0x7F 'E'
    jne not_elf
    cmp word [es:si+2], 0x464C  ; 'L' 'F'
    jne not_elf

    ; Get entry point (offset 0x18)
    mov bx, [es:si+0x18]    ; Low 16 bits of entry
    ; If you're using a 32-bit entry, you need to switch to protected mode.
    ; For now, assume entry is <= 0xFFFF

    ret

not_elf:
    mov si, not_elf_msg
    call print_string
    jmp $

done:
    ret

; Data section (must be after code)
boot_msg db "PS2 x86 Bootloader v1.0", 0x0D, 0x0A, 0
kernel_loaded_msg db "Kernel loaded, transferring control...", 0x0D, 0x0A, 0
load_error_msg db "Error loading kernel!", 0x0D, 0x0A, 0
disk_error_msg db "Disk read error!", 0x0D, 0x0A, 0
boot_drive db 0
kernel_size dw 0
not_elf_msg db "Invalid ELF file!", 0x0D, 0x0A, 0

; Boot signature
times 510 - ($ - $$) db 0
dw 0xAA55