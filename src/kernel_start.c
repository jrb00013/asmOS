/* Kernel entry trampoline — linked at 0x100000 with ASMOS magic. */

#include <stdint.h>
#include "boot_meta.h"

extern void kernel_main(void);
extern void debugcon_puts(const char *s);

/* This must be the very first thing in the kernel image after the 4-byte
 * .bootsig magic, at a fixed, predictable address — the boot loader
 * (boot/loader.asm) jumps to a hardcoded physical address after copying
 * the kernel to 0x100000, and it has no ELF symbol table to consult at
 * that point. A plain ".text" placement puts _kernel_start wherever the
 * linker's default input-file ordering happens to land it (empirically,
 * far into .text, not at the start), so the loader's fixed jump target
 * would land on unrelated code instead. ".text.boot" is referenced first
 * inside the .text output section in llinker/linker.ld, which pins this
 * at a fixed, known offset regardless of what else gets linked in. */
void _kernel_start(void) __attribute__((section(".text.boot"), used));

void _kernel_start(void) {
    debugcon_puts("DEBUG:KERNEL_START\n");
    kernel_main();
    debugcon_puts("DEBUG:KERNEL_HALT\n");
    for (;;);
}
