ENTRY(kernel_main)

MEMORY {
    RAM (rwx) : ORIGIN = 0x10000, LENGTH = 50M
}

SECTIONS {
    . = 0x10000;
    
    /* Read-only sections */
    .text : ALIGN(4K) {
        *(.text)
    } > RAM
    
    .rodata : ALIGN(4K) {
        *(.rodata*)
    } > RAM
    
    /* Read-write sections */
    .data : ALIGN(4K) {
        *(.data)
    } > RAM
    
    .bss : ALIGN(4K) {
        *(.bss)
        *(COMMON)
    } > RAM
    
    /* Stack */
    . = ALIGN(4K);
    _stack_start = . + 4K;  /* 4KB stack */
    
    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
    }
}

/* Define program headers for proper segment permissions */
PHDRS {
    text PT_LOAD FLAGS((1 << 0) | (1 << 2)); /* RX */
    rodata PT_LOAD FLAGS(1 << 2);            /* R */
    data PT_LOAD FLAGS((1 << 1) | (1 << 2)); /* RW */
    bss PT_LOAD FLAGS((1 << 1) | (1 << 2));  /* RW */
}