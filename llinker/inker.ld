ENTRY(_start)

SECTIONS {
    . = 0x1000;  /* Starting address for the kernel */
    .text : {
        *(.text)
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
    }
}
