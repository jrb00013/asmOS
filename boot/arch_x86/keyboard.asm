; PS/2 keyboard scancode set 1 → ASCII and polling helpers.
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global scancode_to_ascii
global keyboard_poll_scancode
global read_key

section .rodata
; scancode (set1) index table for 0x00..0x39
asc_unshift:
    db 0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0
    db 'q','w','e','r','t','y','u','i','o','p','[',']',0,0
    db 'a','s','d','f','g','h','j','k','l',';',39,'`',0,'\'
    db '\','z','x','c','v','b','n','m',',','.','/',0,0
    db 0,0,0,' '
asc_shift:
    db 0,0,'!','@','#','$','%','^','&','*','(',')',0,0
    db 'Q','W','E','R','T','Y','U','I','O','P','{','}',0,0
    db 'A','S','D','F','G','H','J','K','L',':','"',0,0
    db '|','Z','X','C','V','B','N','M','<','>','?',0,0
    db 0,0,0,' '

section .text

; uint8_t scancode_to_ascii(uint8_t sc, uint32_t shift)
scancode_to_ascii:
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    cmp eax, 0x39
    ja .none
    test edx, edx
    jz .un
    mov al, [asc_shift + eax]
    ret
.un:
    mov al, [asc_unshift + eax]
    ret
.none:
    xor al, al
    ret

; int keyboard_poll_scancode(void) — scancode in eax, 0 if none
keyboard_poll_scancode:
    push edx
    mov dx, 0x64
    in al, dx
    test al, 1
    jz .none
    mov dx, 0x60
    in al, dx
    movzx eax, al
    pop edx
    ret
.none:
    xor eax, eax
    pop edx
    ret

; int read_key(void) — blocking ASCII key
read_key:
    push ebx
    push esi
    mov esi, 0
.wait:
    call keyboard_poll_scancode
    test eax, eax
    jz .wait
    mov ebx, eax
    cmp bl, 0x2A
    je .shift_on
    cmp bl, 0x36
    je .shift_on
    cmp bl, 0xAA
    je .shift_off
    cmp bl, 0xB6
    je .shift_off
    cmp bl, 0x1C
    je .enter
    push esi
    mov ecx, ebx
    mov edx, 0
    call scancode_to_ascii
    pop esi
    test al, al
    jz .wait
    movzx eax, al
    jmp .done
.shift_on:
    mov esi, 1
    jmp .wait
.shift_off:
    xor esi, esi
    jmp .wait
.enter:
    mov al, 13
    movzx eax, al
.done:
    pop esi
    pop ebx
    ret
