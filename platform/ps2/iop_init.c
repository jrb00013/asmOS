#ifdef PLATFORM_PS2

#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <debug.h>
#include "kernel.h"

static int iop_ready;

int ps2_iop_init(void) {
    if (iop_ready) return 0;
    SifInitRpc(0);
    while (!SifIopReset("", 0)) { };
    while (!SifIopSync()) { };
    int r;
    r = SifLoadModule("host:irx/smap_irx.irx", 0, NULL);
    if (r < 0) scr_printf("iop: smap load %d\n", r);
    r = SifLoadModule("host:irx/padman_irx.irx", 0, NULL);
    if (r < 0) scr_printf("iop: padman load %d\n", r);
    r = SifLoadModule("host:irx/mcman_irx.irx", 0, NULL);
    if (r < 0) scr_printf("iop: mcman load %d\n", r);
    iop_ready = 1;
    return 0;
}

int ps2_iop_status(char *buf, int max) {
    if (!buf || max < 4) return -1;
    buf[0] = iop_ready ? 'r' : 'd';
    buf[1] = 'e'; buf[2] = 'a'; buf[3] = 'd'; buf[4] = '\0';
    return 0;
}

#endif
