#include "dashboard.h"
#include "kernel.h"
#include "memory_manager.h"
#include <stddef.h>

#define DASHBOARD_BAR_LEN 32

void dashboard_init(void) {
    (void)0;
}

void dashboard_show(void) {
    unsigned int mem_pct = get_memory_usage_percent();
    if (mem_pct > 100) mem_pct = 100;

    kprint("\n  ");
    kprint_color(" dashboard ", 0x2A);
    kprint_color(" ---------------------------------\n", 0x08);
    kprint("  memory  ");
    kprint_color("[", 0x08);
    int filled = (mem_pct * DASHBOARD_BAR_LEN) / 100;
    for (int i = 0; i < DASHBOARD_BAR_LEN; i++) {
        if (i < filled)
            kprint_color("#", 0x0A);
        else
            kprint_color(".", 0x08);
    }
    kprint_color("]", 0x08);
    kprintf(" %u%%\n", mem_pct);
    kprint("  led      (use ");
    kprint_color("led", 0x0B);
    kprint(" command)\n");
    kprint("  network  (no adapter)\n");
    kprint("  party    (no party)\n");
    kprint("  ----------------------------------------\n\n");
}
