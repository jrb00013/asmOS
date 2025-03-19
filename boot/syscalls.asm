[BITS 32]              ; 32-bit mode for system calls
[GLOBAL print_string, exit_program]  ; Make functions globally accessible

section .text

; ============================
; print_string - Print a string to the screen
; Args: 
;    EDI -> Address of the string
; Returns: None
; ============================
print_string:
    pusha                   ; Save registers
    mov eax, 4              ; Syscall number for write (Linux convention)
    mov ebx, 1              ; File descriptor 1 (stdout)
    mov ecx, edi            ; String address
    mov edx, 100            ; Max length (consider replacing with strlen)
    int 0x80                ; Call the interrupt
    popa                    ; Restore registers
    ret

; ============================
; exit_program - Exit the OS/kernel
; Args: 
;    EBX -> Exit code (default 0 if not set)
; Returns: Does not return
; ============================
exit_program:
    mov eax, 1              ; Syscall number for exit
    int 0x80                ; Call the interrupt
    hlt                     ; Halt system in case the syscall fails
