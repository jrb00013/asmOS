/* Kernel entry trampoline — linked at 0x100000 with ASMOS magic. */

#include <stdint.h>
#include "boot_meta.h"

extern void kernel_main(void);

void _kernel_start(void) __attribute__((section(".text"), used));

void _kernel_start(void) {
    kernel_main();
    for (;;);
}
