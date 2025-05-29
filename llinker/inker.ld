ENTRY(kernel_main)

MEMORY {
    RAM (rwx) : ORIGIN = 0x10000, LENGTH = 1M  /* 1MB for kernel */
}

SECTIONS {
    . = 0x10000;
    
    .text : {
        *(.text)
        *(.rodata*)
    } > RAM
    
    .data : {
        *(.data)
    } > RAM
    
    .bss : {
        *(.bss)
        *(COMMON)
    } > RAM
    
    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
    
    /* Stack starts at end of memory and grows down */
    _stack_start = ORIGIN(RAM) + LENGTH(RAM) - 4;
}
