; PS/2 keyboard scancode set 1 → ASCII and polling helpers.
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global scancode_to_ascii
global keyboard_poll_scancode
global read_key

section .rodata
; Set-1 make scancode -> ASCII, indexed directly by the scancode.
; Written one entry per line with the scancode in a trailing comment
; rather than as packed rows: the previous packed rows omitted padding for
; the keys that produce no character (ENTER, bare SHIFT, ALT), which
; shifted every entry after 0x2B up by one -- 'm' decoded as 'n', and
; space (0x39) fell off the end of the table entirely and was dropped.
; A 0 entry means "no character"; scancode_to_ascii turns that back into
; "ignore this keystroke".
asc_unshift:
    db 0,      ; 00
    db 0,      ; 01
    db '1',    ; 02
    db '2',    ; 03
    db '3',    ; 04
    db '4',    ; 05
    db '5',    ; 06
    db '6',    ; 07
    db '7',    ; 08
    db '8',    ; 09
    db '9',    ; 0A
    db '0',    ; 0B
    db '-',    ; 0C
    db '=',    ; 0D
    db 0,      ; 0E backspace
    db 0,      ; 0F tab
    db 'q',    ; 10
    db 'w',    ; 11
    db 'e',    ; 12
    db 'r',    ; 13
    db 't',    ; 14
    db 'y',    ; 15
    db 'u',    ; 16
    db 'i',    ; 17
    db 'o',    ; 18
    db 'p',    ; 19
    db '[',    ; 1A
    db ']',    ; 1B
    db 0,      ; 1C enter
    db 0,      ; 1D left ctrl
    db 'a',    ; 1E
    db 's',    ; 1F
    db 'd',    ; 20
    db 'f',    ; 21
    db 'g',    ; 22
    db 'h',    ; 23
    db 'j',    ; 24
    db 'k',    ; 25
    db 'l',    ; 26
    db ';',    ; 27
    db 39,     ; 28
    db '`',    ; 29
    db 0,      ; 2A left shift
    db '\',    ; 2B backslash
    db 'z',    ; 2C
    db 'x',    ; 2D
    db 'c',    ; 2E
    db 'v',    ; 2F
    db 'b',    ; 30
    db 'n',    ; 31
    db 'm',    ; 32
    db ',',    ; 33
    db '.',    ; 34
    db '/',    ; 35
    db 0,      ; 36 right shift
    db '*',    ; 37 keypad *
    db 0,      ; 38 left alt
    db ' ',    ; 39 space
asc_shift:
    db 0,      ; 00
    db 0,      ; 01
    db '!',    ; 02
    db '@',    ; 03
    db '#',    ; 04
    db '$',    ; 05
    db '%',    ; 06
    db '^',    ; 07
    db '&',    ; 08
    db '*',    ; 09
    db '(',    ; 0A
    db ')',    ; 0B
    db '_',    ; 0C
    db '+',    ; 0D
    db 0,      ; 0E backspace
    db 0,      ; 0F tab
    db 'Q',    ; 10
    db 'W',    ; 11
    db 'E',    ; 12
    db 'R',    ; 13
    db 'T',    ; 14
    db 'Y',    ; 15
    db 'U',    ; 16
    db 'I',    ; 17
    db 'O',    ; 18
    db 'P',    ; 19
    db '{',    ; 1A
    db '}',    ; 1B
    db 0,      ; 1C enter
    db 0,      ; 1D left ctrl
    db 'A',    ; 1E
    db 'S',    ; 1F
    db 'D',    ; 20
    db 'F',    ; 21
    db 'G',    ; 22
    db 'H',    ; 23
    db 'J',    ; 24
    db 'K',    ; 25
    db 'L',    ; 26
    db ':',    ; 27
    db '"',    ; 28
    db '~',    ; 29
    db 0,      ; 2A left shift
    db '|',    ; 2B backslash
    db 'Z',    ; 2C
    db 'X',    ; 2D
    db 'C',    ; 2E
    db 'V',    ; 2F
    db 'B',    ; 30
    db 'N',    ; 31
    db 'M',    ; 32
    db '<',    ; 33
    db '>',    ; 34
    db '?',    ; 35
    db 0,      ; 36 right shift
    db 0,      ; 37 keypad *
    db 0,      ; 38 left alt
    db ' ',    ; 39 space

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
