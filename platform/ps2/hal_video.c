#include "platform.h"
#include "video.h"
#include <graph.h>
#include <draw.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <kernel.h>

static int gfx_inited;
static u32 frame_addr;
static int fb_width = 640;
static int fb_height = 448;

void plat_video_mode_text(void) {
    scr_clear();
}

void plat_video_mode_13h(void) {
    if (gfx_inited) return;
    frame_addr = GRAPH_ADDR;
    fb_width = 320;
    fb_height = 256;
    graph_set_mode(GS_MODE_NTSC, GS_MODE_INTERLACED, GS_MODE_FIELD);
    graph_set_screen(0, 0, 320, 256);
    graph_set_bgcolor(0, 0, 0);
    graph_set_framebuffer_filtered(frame_addr, fb_width, PSMT_RGBA32, 0, 0);
    graph_enable_output();
    dma_channel_wait(DMA_CHANNEL_GIF, 0);
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
