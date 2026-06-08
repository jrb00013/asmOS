/* x86 platform HAL — VGA video modes. */

#include "platform.h"
#include "video.h"
#include <stdint.h>

void plat_video_mode_text(void) {
    video_set_mode_text();
}

void plat_video_mode_13h(void) {
    video_set_mode_13h();
}

void plat_video_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    video_set_palette_entry(index, r, g, b);
}

volatile uint8_t *plat_framebuffer(void) {
    return (volatile uint8_t *)FRAMEBUFFER_ADDR;
}
