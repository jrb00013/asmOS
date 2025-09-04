#include "graphics.h"
#include "kernel.h"
#include <stdint.h>

// Graphics demo system for PS2
typedef struct {
    uint32_t x, y;
    uint32_t width, height;
    uint32_t color;
    uint32_t velocity_x, velocity_y;
} demo_object_t;

// Demo objects
static demo_object_t objects[10];
static uint32_t object_count = 0;
static uint32_t demo_running = 0;

// Initialize graphics demo system
void init_graphics_demo(void) {
    kprint("Initializing PS2 Graphics Demo System...\n");
    
    // Initialize graphics hardware
    asm volatile("call sys_graphics_init");
    
    // Create demo objects
    object_count = 5;
    for (int i = 0; i < object_count; i++) {
        objects[i].x = 100 + (i * 50);
        objects[i].y = 100 + (i * 30);
        objects[i].width = 30 + (i * 5);
        objects[i].height = 30 + (i * 5);
        objects[i].color = 0xFF0000 + (i * 0x001100); // Red to blue gradient
        objects[i].velocity_x = 2 + i;
        objects[i].velocity_y = 1 + i;
    }
    
    demo_running = 1;
    kprint("Graphics demo system initialized!\n");
}

// Run graphics demo
void run_graphics_demo(void) {
    if (!demo_running) {
        kprint("Graphics demo not initialized!\n");
        return;
    }
    
    kprint("Running PS2 Graphics Demo...\n");
    kprint("Press any key to stop demo\n");
    
    uint32_t frame_count = 0;
    
    while (demo_running && frame_count < 1000) {
        // Clear screen
        clear_graphics_screen();
        
        // Update and draw objects
        for (int i = 0; i < object_count; i++) {
            // Update position
            objects[i].x += objects[i].velocity_x;
            objects[i].y += objects[i].velocity_y;
            
            // Bounce off walls
            if (objects[i].x <= 0 || objects[i].x >= 640 - objects[i].width) {
                objects[i].velocity_x = -objects[i].velocity_x;
            }
            if (objects[i].y <= 0 || objects[i].y >= 480 - objects[i].height) {
                objects[i].velocity_y = -objects[i].velocity_y;
            }
            
            // Draw object
            draw_rectangle(objects[i].x, objects[i].y, 
                          objects[i].width, objects[i].height, 
                          objects[i].color);
        }
        
        // Draw demo info
        draw_text(10, 10, "PS2 Graphics Demo", 0xFFFFFF);
        draw_text(10, 30, "Frame:", 0xFFFFFF);
        draw_number(80, 30, frame_count, 0xFFFFFF);
        
        // Check for key press to stop
        if (check_key_press()) {
            demo_running = 0;
        }
        
        frame_count++;
        
        // Small delay
        for (volatile int i = 0; i < 10000; i++);
    }
    
    kprint("Graphics demo completed!\n");
}

// Clear graphics screen
void clear_graphics_screen(void) {
    // Clear screen with black
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
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

// Draw line
void draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);
    
    float x_inc = (float)dx / steps;
    float y_inc = (float)dy / steps;
    
    float x = x1;
    float y = y1;
    
    for (int i = 0; i <= steps; i++) {
        set_pixel((int)x, (int)y, color);
        x += x_inc;
        y += y_inc;
    }
}

// Draw text
void draw_text(uint32_t x, uint32_t y, const char* text, uint32_t color) {
    // Simple 8x8 font rendering
    for (int i = 0; text[i] != '\0'; i++) {
        draw_char(x + (i * 8), y, text[i], color);
    }
}

// Draw character
void draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    // Simple 8x8 font data (simplified)
    static const uint8_t font_data[128][8] = {
        // Basic ASCII characters
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
        {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00}, // !
        // Add more character definitions...
    };
    
    uint8_t char_data[8];
    if (c < 128) {
        for (int i = 0; i < 8; i++) {
            char_data[i] = font_data[c][i];
        }
    }
    
    // Draw character
    for (int py = 0; py < 8; py++) {
        for (int px = 0; px < 8; px++) {
            if (char_data[py] & (0x80 >> px)) {
                set_pixel(x + px, y + py, color);
            }
        }
    }
}

// Draw number
void draw_number(uint32_t x, uint32_t y, uint32_t number, uint32_t color) {
    char buffer[16];
    int_to_string(number, buffer);
    draw_text(x, y, buffer, color);
}

// Set pixel
void set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= 640 || y >= 480) return;
    
    // Calculate VRAM address
    uint32_t vram_addr = 0x70000000 + (y * 640 + x) * 4;
    *(uint32_t*)vram_addr = color;
}

// Check for key press
int check_key_press(void) {
    uint32_t key_data;
    asm volatile("call sys_ps2_controller_read" : "=a"(key_data));
    return (key_data & 0x00000001) != 0;
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
