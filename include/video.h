#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

/* Framebuffer configuration. On PC we use VGA mode 13h; on PS2 we use GS VRAM. */
#ifdef PS2_HARDWARE
#define FRAMEBUFFER_ADDR   0x70000000
#define VIDEO_WIDTH        640
#define VIDEO_HEIGHT       480
#define VIDEO_BPP          4
#else
#define FRAMEBUFFER_ADDR   0xA0000
#define VIDEO_WIDTH        320
#define VIDEO_HEIGHT       200
#define VIDEO_BPP          1
#endif

/* Switch to VGA mode 13h (320x200, 256 colors). No-op when PS2_HARDWARE. */
void video_set_mode_13h(void);

/* Switch back to VGA text mode 0x03 (80x25). No-op when PS2_HARDWARE. */
void video_set_mode_text(void);

/* Set VGA palette entry (mode 13h only). index 0..255, r/g/b 0..63. */
void video_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

#endif
