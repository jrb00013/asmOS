OUTPUT_FORMAT(elf32-i386)
ENTRY(_stage2_start)

SECTIONS
{
    . = 0x90000;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) *(COMMON) }
    _stage2_end = .;
}
