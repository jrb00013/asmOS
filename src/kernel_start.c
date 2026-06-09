/* Kernel entry trampoline — linked at 0x100000 with ASMOS magic. */

#include <stdint.h>
#include "boot_meta.h"

extern void kernel_main(void);
extern void debugcon_puts(const char *s);

void _kernel_start(void) __attribute__((section(".text"), used));

void _kernel_start(void) {
    debugcon_puts("DEBUG:KERNEL_START\n");
    kernel_main();
    debugcon_puts("DEBUG:KERNEL_HALT\n");
    for (;;);
}
