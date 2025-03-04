; syscalls.asm
; Implement basic system calls for the OS (e.g., print, exit, etc.)

[bits 32]              ; 32-bit mode for system calls
[global _start]        ; Make _start the entry point

_start:
    ; System call to print a string
    ; Args: EDI -> string address
    ; Interrupt number: 0x80
print_string:
    mov eax, 4          ; syscall number for write (Linux convention)
    mov ebx, 1          ; file descriptor 1 (stdout)
    mov edx, 100        ; max length
    int 0x80            ; make the syscall
    ret

    ; System call to exit the program
exit_program:
    mov eax, 1          ; syscall number for exit
    xor ebx, ebx        ; exit code 0
    int 0x80            ; make the syscall
    ret
