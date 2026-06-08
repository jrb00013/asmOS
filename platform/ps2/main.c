/*
 * ASMOS PS2 native entry — FreeMCBoot / uLaunchELF compatible EE ELF.
 */

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <debug.h>
#include <fileio.h>
#include <pad.h>

extern void kernel_main(void);

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    SifInitRpc(0);
    init_scr();
    scr_printf("\nASMOS PS2 Native Boot\n");

    if (fioInit() < 0)
        scr_printf("Warning: fioInit failed\n");

    if (padInit(0) != 1)
        scr_printf("Warning: padInit failed\n");

    kernel_main();

    scr_printf("ASMOS halted.\n");
    SleepThread();
    return 0;
}
