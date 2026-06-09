#include "platform.h"
#include "video.h"
#include <graph.h>
#include <graph_vram.h>
#include <draw.h>
#include <gs_psm.h>
#include <debug.h>
#include <kernel.h>
#include <stdint.h>

static int gfx_inited;
static u32 frame_addr;
static int fb_width = 640;
static int fb_height = 448;

void plat_video_mode_text(void) {
    scr_clear();
}

void plat_video_mode_13h(void) {
    if (gfx_inited) return;
    fb_width = 320;
    fb_height = 256;
    frame_addr = (u32)graph_vram_allocate(fb_width, fb_height, GS_PSM_32, GRAPH_ALIGN_PAGE);
    graph_initialize((int)frame_addr, fb_width, fb_height, GS_PSM_32, 0, 0);
    graph_enable_output();
    gfx_inited = 1;
}

void plat_video_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    (void)index;
    (void)r;
    (void)g;
    (void)b;
}

volatile uint8_t *plat_framebuffer(void) {
    return (volatile uint8_t *)(frame_addr << 8);
}
