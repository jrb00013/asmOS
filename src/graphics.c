#include "graphics.h"
#include "kernel.h"
#include "video.h"
#include "font_8x8.h"
#ifndef PLATFORM_PS2
#include "keyboard.h"
#endif
#include "syscalls.h"
#include <stdint.h>

/* Demo objects */
static demo_object_t objects[10];
static uint32_t object_count = 0;
static uint32_t demo_running = 0;

#if defined(PS2_HARDWARE) && defined(PLATFORM_PS2)
#include "platform.h"
static void init_graphics_hw(void) {
    plat_video_mode_text();
}
#elif defined(PS2_HARDWARE)
static void init_graphics_hw(void) {
    sys_graphics_init();
}
#else
static void set_palette_16(void) {
    video_set_palette_entry(0,  0, 0, 0);
    video_set_palette_entry(1,  63, 0, 0);
    video_set_palette_entry(2,  0, 63, 0);
    video_set_palette_entry(3,  0, 0, 63);
    video_set_palette_entry(4,  63, 63, 0);
    video_set_palette_entry(5,  0, 63, 63);
    video_set_palette_entry(6,  63, 0, 63);
    video_set_palette_entry(7,  63, 63, 63);
    for (int i = 8; i < 16; i++) {
        int v = (i - 8) * 8;
        if (v > 63) v = 63;
        video_set_palette_entry((uint8_t)i, (uint8_t)v, (uint8_t)v, (uint8_t)v);
    }
}
static void init_graphics_hw(void) {
    set_palette_16();
}
#endif

/* Map 24-bit RGB to palette index 0..15 for mode 13h */
static uint8_t color_to_palette(uint32_t rgb) {
    if (rgb == 0x000000) return 0;
    if (rgb == 0xFF0000) return 1;
    if (rgb == 0x00FF00) return 2;
    if (rgb == 0x0000FF) return 3;
    if (rgb == 0xFFFF00) return 4;
    if (rgb == 0x00FFFF) return 5;
    if (rgb == 0xFF00FF) return 6;
    if (rgb == 0xFFFFFF) return 7;
    /* Other colors: use luminance and map to 8..15 */
    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8) & 0xFF;
    uint32_t b = rgb & 0xFF;
    uint32_t lum = (r + g + b) / 3;
    return (uint8_t)(8 + (lum * 7) / 255);
}

#ifndef PS2_HARDWARE
void graphics_set_palette(void) {
    set_palette_16();
}
#else
void graphics_set_palette(void) { (void)0; }
#endif

void init_graphics_demo(void) {
    kprint("Initializing Graphics Demo System...\n");
    init_graphics_hw();

    object_count = 5;
    for (int i = 0; i < object_count; i++) {
        objects[i].x = 40 + (i * 25);
        objects[i].y = 40 + (i * 15);
        objects[i].width = 12 + (i * 2);
        objects[i].height = 12 + (i * 2);
        objects[i].color = 0xFF0000 + (i * 0x001100);
        objects[i].velocity_x = 1 + (i & 1);
        objects[i].velocity_y = 1 + ((i >> 1) & 1);
    }
    demo_running = 1;
    kprint("Graphics demo system initialized!\n");
}

void run_graphics_demo(void) {
    if (!demo_running) {
        kprint("Graphics demo not initialized!\n");
        return;
    }

    kprint("Running Graphics Demo - press any key to stop\n");

    uint32_t frame_count = 0;
    while (demo_running && frame_count < 2000) {
        clear_graphics_screen();

        for (uint32_t i = 0; i < object_count; i++) {
            objects[i].x += objects[i].velocity_x;
            objects[i].y += objects[i].velocity_y;

            if (objects[i].x <= 0 || objects[i].x >= VIDEO_WIDTH - objects[i].width) {
                int32_t vx = (int32_t)objects[i].velocity_x;
                objects[i].velocity_x = (uint32_t)(-vx);
            }
            if (objects[i].y <= 0 || objects[i].y >= VIDEO_HEIGHT - objects[i].height) {
                int32_t vy = (int32_t)objects[i].velocity_y;
                objects[i].velocity_y = (uint32_t)(-vy);
            }

            draw_rectangle(objects[i].x, objects[i].y,
                           objects[i].width, objects[i].height,
                           objects[i].color);
        }

        draw_text(5, 5, "Graphics Demo", 0xFFFFFF);
        draw_text(5, 15, "Frame:", 0xFFFFFF);
        draw_number(50, 15, frame_count, 0xFFFFFF);

        if (check_key_press())
            demo_running = 0;

        frame_count++;
        for (volatile int d = 0; d < 15000; d++) ;
    }

    kprint("Graphics demo completed!\n");
}

void clear_graphics_screen(void) {
    for (uint32_t y = 0; y < VIDEO_HEIGHT; y++) {
        for (uint32_t x = 0; x < VIDEO_WIDTH; x++) {
            set_pixel(x, y, 0x000000);
        }
    }
}

// Draw rectangle
void draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    for (uint32_t py = y; py < y + height; py++) {
        for (uint32_t px = x; px < x + width; px++) {
            set_pixel(px, py, color);
        }
    }
}

// Draw circle
void draw_circle(uint32_t x, uint32_t y, uint32_t radius, uint32_t color) {
    for (int py = y - radius; py <= y + radius; py++) {
        for (int px = x - radius; px <= x + radius; px++) {
            int dx = px - x;
            int dy = py - y;
            if (dx * dx + dy * dy <= radius * radius) {
                set_pixel(px, py, color);
            }
        }
    }
}

/* Integer Bresenham line (no floating point) */
void draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    int dx = (int)(x2 - x1);
    int dy = (int)(y2 - y1);
    int ax = dx < 0 ? -dx : dx;
    int ay = dy < 0 ? -dy : dy;
    int sx = dx < 0 ? -1 : 1;
    int sy = dy < 0 ? -1 : 1;

    int x = (int)x1;
    int y = (int)y1;

    if (ax > ay) {
        int err = ax / 2;
        while (1) {
            set_pixel((uint32_t)x, (uint32_t)y, color);
            if (x == (int)x2) break;
            err -= ay;
            if (err < 0) { y += sy; err += ax; }
            x += sx;
        }
    } else {
        int err = ay / 2;
        while (1) {
            set_pixel((uint32_t)x, (uint32_t)y, color);
            if (y == (int)y2) break;
            err -= ax;
            if (err < 0) { x += sx; err += ay; }
            y += sy;
        }
    }
}

// Draw text
void draw_text(uint32_t x, uint32_t y, const char* text, uint32_t color) {
    // Simple 8x8 font rendering
    for (int i = 0; text[i] != '\0'; i++) {
        draw_char(x + (i * 8), y, text[i], color);
    }
}

void draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    unsigned int ch = (unsigned char)c & 0x7F;
    for (int py = 0; py < 8; py++) {
        uint8_t line = font_8x8[ch][py];
        for (int px = 0; px < 8; px++) {
            if (line & (0x80u >> px))
                set_pixel(x + px, y + py, color);
        }
    }
}

// Draw number
void draw_number(uint32_t x, uint32_t y, uint32_t number, uint32_t color) {
    char buffer[16];
    int_to_string(number, buffer);
    draw_text(x, y, buffer, color);
}

void set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= VIDEO_WIDTH || y >= VIDEO_HEIGHT) return;

#if defined(PS2_HARDWARE)
    {
        uint32_t vram_addr = FRAMEBUFFER_ADDR + (y * VIDEO_WIDTH + x) * 4;
        *(uint32_t *)vram_addr = color;
    }
#else
    {
        uint8_t idx = color_to_palette(color);
        uint32_t off = y * VIDEO_WIDTH + x;
        *(uint8_t *)(FRAMEBUFFER_ADDR + off) = idx;
    }
#endif
}

int check_key_press(void) {
#ifdef PLATFORM_PS2
    uint8_t pad[6];
    sys_ps2_controller_read(pad);
    return (pad[0] | pad[1]) != 0;
#elif defined(PS2_HARDWARE)
    return keyboard_has_key();
#else
    return keyboard_has_key();
#endif
}

// Convert integer to string
void int_to_string(uint32_t num, char* buffer) {
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    int i = 0;
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // Reverse string
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - 1 - j];
        buffer[i - 1 - j] = temp;
    }
    
    buffer[i] = '\0';
}

// Stop graphics demo
void stop_graphics_demo(void) {
    demo_running = 0;
    kprint("Graphics demo stopped\n");
}
