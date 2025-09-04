#include "msp.h"
#include "shell.h"
#include "memory_manager.h"
#include "scheduler.h"
#include "fs.h"  // New filesystem header
#include "kernel.h" 
#include <stdarg.h>
#include <stdint.h>

// Assembly function prototypes
extern void enable_interrupts_asm(void);
extern void disable_interrupts_asm(void);
extern void load_kernel_from_disk(void);

// PS2-specific hardware detection
extern uint32_t detect_ps2_memory(void);
extern void init_ps2_controllers(void);

// VGA text buffer starts at 0xB8000
volatile uint16_t* vga_buffer = (uint16_t*)VGA_ADDRESS;
int cursor_row = 0;
int cursor_col = 0;

// PS2 system information
static struct {
    uint32_t total_memory;
    uint8_t ps2_detected;
    uint8_t dualshock_support;
    uint8_t network_support;
} ps2_info = {0};

// Enhanced kernel entry point with PS2 optimizations
void kernel_main(void) {
    // Initialize critical components first
    kprint("PS2 x86 OS Kernel v2.0 - Enhanced Edition\n");  
    disable_interrupts_asm();
    
    // Detect PS2 hardware
    kprint("Detecting PS2 hardware...\n");
    ps2_info.total_memory = detect_ps2_memory();
    ps2_info.ps2_detected = (ps2_info.total_memory >= 32);
    
    if (ps2_info.ps2_detected) {
        kprintf("PS2 detected! Total memory: %u MB\n", ps2_info.total_memory);
        init_ps2_controllers();
    } else {
        kprint("Running in compatibility mode\n");
    }
    
    kprint("Initializing memory manager...\n");
    init_memory_manager();
    
    kprint("Loading kernel from disk...\n");
    load_kernel_from_disk();
    
    // Initialize system components
    kprint("Initializing scheduler and FAT12 filesystem...\n");
    init_scheduler();
    init_fat12();  // Initialize filesystem
    
    kprint("Initializing enhanced shell...\n");
    init_shell();

    kprint("Showing enhanced boot splash...\n");
    show_enhanced_boot_splash(); 
    
    // Enable interrupts and start system
    enable_interrupts_asm();
    kprint("PS2 x86 OS Kernel v2.0 - Ready!\n");
    
    start_shell();
    // Catch
    halt_system();
}

// System halt function
void halt_system(void) {
    kprint("System halted. Press any key to reboot...\n");
    // Wait for key press
    asm volatile("int $0x16");
    // Reboot
    asm volatile("int $0x19");
}

// Get system memory function
uint32_t get_system_memory(void) {
    return ps2_info.total_memory;
}

static void draw_box(int x, int y, int w, int h, uint8_t color) {
    for (int i = x + 1; i < x + w - 1; i++) {
        vga_buffer[y * VGA_WIDTH + i] = ((uint16_t)color << 8) | 0x2500; // ─
        vga_buffer[(y + h - 1) * VGA_WIDTH + i] = ((uint16_t)color << 8) | 0x2500; // ─
    }
    for (int i = y + 1; i < y + h - 1; i++) {
        vga_buffer[i * VGA_WIDTH + x] = ((uint16_t)color << 8) | 0x2502; // │
        vga_buffer[i * VGA_WIDTH + x + w - 1] = ((uint16_t)color << 8) | 0x2502; // │
    }
    vga_buffer[y * VGA_WIDTH + x] = ((uint16_t)color << 8) | 0x250C; // ┌
    vga_buffer[y * VGA_WIDTH + x + w - 1] = ((uint16_t)color << 8) | 0x2510; // ┐
    vga_buffer[(y + h - 1) * VGA_WIDTH + x] = ((uint16_t)color << 8) | 0x2514; // └
    vga_buffer[(y + h - 1) * VGA_WIDTH + x + w - 1] = ((uint16_t)color << 8) | 0x2518; // ┘
}

static void print_at(int x, int y, const char* str) {
    int idx = y * VGA_WIDTH + x;
    while (*str) {
        vga_buffer[idx++] = ((uint16_t)DEFAULT_COLOR << 8) | *str++;
    }
}

static void draw_progress_bar(int x, int y, int width, int percent) {
    int filled = (width * percent) / 100;
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            vga_buffer[y * VGA_WIDTH + x + i] = ((uint16_t)0x2A << 8) | '#'; // Bright color (color 0x2A)
        } else {
            vga_buffer[y * VGA_WIDTH + x + i] = ((uint16_t)DEFAULT_COLOR << 8) | '-';
        }
    }
}

// Enhanced boot splash with PS2 branding
void show_enhanced_boot_splash(void) {
    // Clear screen
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = ((uint16_t)DEFAULT_COLOR << 8) | ' ';
    }

    // Draw enhanced PS2-themed boot screen
    draw_box(5, 3, 70, 20, 0x1F);  // Blue border
    
    // PS2 logo/header
    print_at(25, 4, "PS2 x86 Operating System v2.0");
    print_at(20, 5, "Enhanced Edition for PlayStation 2");
    
    print_at(15, 7, "System Information:");
    if (ps2_info.ps2_detected) {
        print_at(15, 8, "PS2 Hardware: DETECTED");
        kprintf("Memory: %u MB available", ps2_info.total_memory);
    } else {
        print_at(15, 8, "PS2 Hardware: NOT DETECTED (Compatibility Mode)");
    }

    print_at(15, 10, "Initializing system components...");

    // Memory manager progress
    print_at(15, 11, "Loading memory manager...");
    draw_progress_bar(15, 12, 40, 0);
    for (int p = 0; p <= 100; p += 20) {
        draw_progress_bar(15, 12, 40, p);
        for (volatile int i=0; i<800000; i++);  // PS2-optimized delay
    }
    print_at(15, 13, "Memory manager initialized");

    // Filesystem progress
    print_at(15, 14, "Loading FAT12 filesystem...");
    draw_progress_bar(15, 15, 40, 0);
    for (int p = 0; p <= 100; p += 25) {
        draw_progress_bar(15, 15, 40, p);
        for (volatile int i=0; i<800000; i++);
    }
    print_at(15, 16, "Filesystem initialized");

    // Scheduler progress
    print_at(15, 17, "Loading task scheduler...");
    draw_progress_bar(15, 18, 40, 0);
    for (int p = 0; p <= 100; p += 33) {
        draw_progress_bar(15, 18, 40, p);
        for (volatile int i=0; i<800000; i++);
    }
    print_at(15, 19, "Scheduler initialized");

    print_at(15, 20, "Starting enhanced shell...");
    for (volatile int i=0; i<1000000; i++);
    
    // Clear screen for shell
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = ((uint16_t)DEFAULT_COLOR << 8) | ' ';
    }
    cursor_row = 0;
    cursor_col = 0;
}

void kprint_char(char c) {
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
    } else {
        int index = cursor_row * VGA_WIDTH + cursor_col;
        vga_buffer[index] = ((uint16_t)DEFAULT_COLOR << 8) | c;
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }

    // Scroll if necessary
    if (cursor_row >= VGA_HEIGHT) {
        for (int row = 1; row < VGA_HEIGHT; row++) {
            for (int col = 0; col < VGA_WIDTH; col++) {
                vga_buffer[(row - 1) * VGA_WIDTH + col] = vga_buffer[row * VGA_WIDTH + col];
            }
        }
        for (int col = 0; col < VGA_WIDTH; col++) {
            vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = (uint16_t)DEFAULT_COLOR << 8 | ' ';
        }
        cursor_row = VGA_HEIGHT - 1;
    }
}

void kprint(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        kprint_char(str[i]);
    }
}


void kprint_char_color(char c, uint8_t color) {
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
    } else {
        int index = cursor_row * VGA_WIDTH + cursor_col;
        vga_buffer[index] = ((uint16_t)color << 8) | c;
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }

    // Scroll like in kprint_char
    if (cursor_row >= VGA_HEIGHT) {
        for (int row = 1; row < VGA_HEIGHT; row++) {
            for (int col = 0; col < VGA_WIDTH; col++) {
                vga_buffer[(row - 1) * VGA_WIDTH + col] = vga_buffer[row * VGA_WIDTH + col];
            }
        }
        for (int col = 0; col < VGA_WIDTH; col++) {
            vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = ((uint16_t)DEFAULT_COLOR << 8) | ' ';
        }
        cursor_row = VGA_HEIGHT - 1;
    }
}


void kprint_color(const char *str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        kprint_char_color(str[i], color);
    }
}



void putchar(char c) {
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
    } else {
        int idx = cursor_row * VGA_WIDTH + cursor_col;
        vga_buffer[idx] = ((uint16_t)DEFAULT_COLOR << 8) | c;
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }

    if (cursor_row >= VGA_HEIGHT) {
        for (int row = 1; row < VGA_HEIGHT; row++) {
            for (int col = 0; col < VGA_WIDTH; col++) {
                vga_buffer[(row - 1) * VGA_WIDTH + col] = vga_buffer[row * VGA_WIDTH + col];
            }
        }
        for (int col = 0; col < VGA_WIDTH; col++) {
            vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = ((uint16_t)DEFAULT_COLOR << 8) | ' ';
        }
        cursor_row = VGA_HEIGHT - 1;
    }
}

void print_decimal(int value) {
    char buffer[16];
    int i = 0;
    if (value == 0) {
        putchar('0');
        return;
    }
    if (value < 0) {
        putchar('-');
        value = -value;
    }
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    while (i--) putchar(buffer[i]);
}

void print_hex(unsigned int value) {
    const char* hex = "0123456789ABCDEF";
    putchar('0');
    putchar('x');
    for (int i = 28; i >= 0; i -= 4) {
        putchar(hex[(value >> i) & 0xF]);
    }
}

void kprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    for (const char* ptr = format; *ptr != '\0'; ptr++) {
        if (*ptr == '%') {
            ptr++;
            switch (*ptr) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    print_string(str);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putchar(c);
                    break;
                }
                case 'd': {
                    int val = va_arg(args, int);
                    print_decimal(val);
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    print_hex(val);
                    break;
                }
                case '%': {
                    putchar('%');
                    break;
                }
                default:
                    putchar('?');
            }
        } else {
            putchar(*ptr);
        }
    }

    va_end(args);
}


int ksstrcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int ksscanf(const char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);

    int count = 0;
    while (*format && *str) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd': {
                    int *out = va_arg(args, int *);
                    int val = 0;
                    int sign = 1;
                    if (*str == '-') {
                        sign = -1;
                        str++;
                    }
                    while (*str >= '0' && *str <= '9') {
                        val = val * 10 + (*str - '0');
                        str++;
                    }
                    *out = val * sign;
                    count++;
                    break;
                }
                case 'x': {
                    int *out = va_arg(args, int *);
                    int val = 0;
                    while ((*str >= '0' && *str <= '9') ||
                           (*str >= 'a' && *str <= 'f') ||
                           (*str >= 'A' && *str <= 'F')) {
                        val *= 16;
                        if (*str >= '0' && *str <= '9') val += *str - '0';
                        else if (*str >= 'a' && *str <= 'f') val += *str - 'a' + 10;
                        else if (*str >= 'A' && *str <= 'F') val += *str - 'A' + 10;
                        str++;
                    }
                    *out = val;
                    count++;
                    break;
                }
                case 'c': {
                    char *out = va_arg(args, char *);
                    *out = *str++;
                    count++;
                    break;
                }
                case 's': {
                    char *out = va_arg(args, char *);
                    while (*str && *str != ' ' && *str != '\n') {
                        *out++ = *str++;
                    }
                    *out = '\0';
                    count++;
                    break;
                }
            }
        } else {
            // Match literal characters
            if (*format != *str) {
                break;
            }
            str++;
        }
        format++;
    }

    va_end(args);
    return count;
}
