[BITS 32]

global print_string
global exit_program
global read_key
global get_memory_info

section .text

; ============================
; print_string - Print a string to screen
; Args: ESI -> Null-terminated string
; ============================

.print_loop:
    lodsb                   ; Load next character
    test al, al             ; Check for null terminator
    jz .print_done
    int 0x10                ; BIOS video interrupt
    jmp .print_loop
.print_done:
    popa
    ret

; ============================
; read_key - Read a key from PS2 keyboard
; Returns: AL -> ASCII character
; ============================
read_key:
    push ebx
    mov ah, 0x00
    int 0x16                ; BIOS keyboard interrupt
    pop ebx
    ret

; ============================
; get_memory_info - Get memory map (PS2-specific)
; Args: EDI -> Buffer for memory map
; Returns: EAX -> Number of memory entries
; ============================
get_memory_info:
    push es
    push di
    push ecx
    push edx
    
    xor ebx, ebx            ; Continuation value
    mov edx, 'PAMS'         ; SMAP signature
    xor ecx, ecx            ; Entry count
    
.mem_loop:
    mov eax, 0xE820         ; BIOS memory map function
    mov ecx, 24             ; Buffer size
    int 0x15
    jc .mem_done            ; CF set on error/last entry
    
    cmp eax, 'PAMS'         ; Verify signature
    jne .mem_done
    
    add di, 24              ; Next buffer position
    inc ecx                 ; Increment entry count
    
    test ebx, ebx           ; Check if more entries
    jnz .mem_loop
    
.mem_done:
    mov eax, ecx            ; Return entry count
    pop edx
    pop ecx
    pop di
    pop es
    ret

; ============================
; exit_program - Exit to shell
; Args: EBX -> Exit code
; ============================
exit_program:
    mov eax, 0x4C00         ; DOS exit function
    or eax, ebx             ; Combine with exit code
    int 0x21                ; DOS interrupt
    hlt
