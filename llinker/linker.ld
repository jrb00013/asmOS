ENTRY(_kernel_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x100000, LENGTH = 31M
}

SECTIONS {
    . = 0x100000;

    .bootsig : {
        LONG(0x41534D4B);
    }

    .text : ALIGN(4K) {
        *(.text)
    } > RAM :text

    .rodata : ALIGN(4K) {
        *(.rodata*)
    } > RAM :rodata

    .data : ALIGN(4K) {
        *(.data)
    } > RAM :data

    .bss : ALIGN(4K) {
        *(.bss)
        *(COMMON)
    } > RAM :bss

    . = ALIGN(4K);
    _stack_start = . + 16K;

    /DISCARD/ : {
        *(.eh_frame)
        *(.note*)
        *(.comment)
    }
}

PHDRS {
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
    bss PT_LOAD FLAGS(6);
}
