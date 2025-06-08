ENTRY(kernel_main)

MEMORY {
    RAM (rwx) : ORIGIN = 0x10000, LENGTH = 50M
}

SECTIONS {
    . = 0x10000;

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
    _stack_start = . + 4K;

    /DISCARD/ : {
        *(.eh_frame)
        *(.note*)
        *(.comment)
    }
}

PHDRS {
    text PT_LOAD FLAGS(5);   /* R-X */
    rodata PT_LOAD FLAGS(4); /* R-- */
    data PT_LOAD FLAGS(6);   /* RW- */
    bss PT_LOAD FLAGS(6);    /* RW- */
}
