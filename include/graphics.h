#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

// Graphics resolution
#define GRAPHICS_WIDTH  640
#define GRAPHICS_HEIGHT 480

// Color definitions
#define COLOR_BLACK     0x000000
#define COLOR_WHITE     0xFFFFFF
#define COLOR_RED       0xFF0000
#define COLOR_GREEN     0x00FF00
#define COLOR_BLUE      0x0000FF
#define COLOR_YELLOW    0xFFFF00
#define COLOR_CYAN      0x00FFFF
#define COLOR_MAGENTA   0xFF00FF

// Graphics demo functions
void init_graphics_demo(void);
void run_graphics_demo(void);
void stop_graphics_demo(void);

// Drawing primitives
void clear_graphics_screen(void);
void set_pixel(uint32_t x, uint32_t y, uint32_t color);
void draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void draw_circle(uint32_t x, uint32_t y, uint32_t radius, uint32_t color);
void draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);

// Text rendering
void draw_text(uint32_t x, uint32_t y, const char* text, uint32_t color);
void draw_char(uint32_t x, uint32_t y, char c, uint32_t color);
void draw_number(uint32_t x, uint32_t y, uint32_t number, uint32_t color);

// Utility functions
int check_key_press(void);
void int_to_string(uint32_t num, char* buffer);

// Graphics demo object structure
typedef struct {
    uint32_t x, y;
    uint32_t width, height;
    uint32_t color;
    uint32_t velocity_x, velocity_y;
} demo_object_t;

#endif // GRAPHICS_H
