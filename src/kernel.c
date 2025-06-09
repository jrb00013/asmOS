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


// VGA text buffer starts at 0xB8000
volatile uint16_t* vga_buffer = (uint16_t*)VGA_ADDRESS;
int cursor_row = 0;
int cursor_col = 0;


// Kernel entry point with proper initialization sequence
void kernel_main(void) {
    // Initialize critical components first
    disable_interrupts_asm();
    init_memory_manager();
    load_kernel_from_disk();
    
    // Initialize system components
    init_scheduler();
    init_fat12();  // Initialize filesystem
    init_shell();

    show_boot_splash(); 
    
    // enable interrupts and start system
    enable_interrupts_asm();
    kprint("PS2 x86 OS Kernel v1.0\n");
    
    start_shell();
    // Catch
    halt_system();
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

void show_boot_splash(void) {
    // Clear screen
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = ((uint16_t)DEFAULT_COLOR << 8) | ' ';
    }

    draw_box(10, 5, 60, 14, DEFAULT_COLOR);
    print_at(15, 6, "PS2 x86 OS Kernel v1.0");
    print_at(15, 8, "Booting system...");

    print_at(15, 9, "Loading memory manager...");
    draw_progress_bar(15, 10, 40, 0);
    for (int p = 0; p <= 100; p += 20) {
        draw_progress_bar(15, 10, 40, p);
        for (volatile int i=0; i<1000000; i++);  // crude delay
    }
    print_at(15, 11, "Memory manager initialized");

    print_at(15, 12, "Loading filesystem...");
    draw_progress_bar(15, 13, 40, 0);
    for (int p = 0; p <= 100; p += 25) {
        draw_progress_bar(15, 13, 40, p);
        for (volatile int i=0; i<1000000; i++);
    }
    print_at(15, 14, "Filesystem initialized");

    print_at(15, 15, "Loading scheduler...");
    draw_progress_bar(15, 16, 40, 0);
    for (int p = 0; p <= 100; p += 33) {
        draw_progress_bar(15, 16, 40, p);
        for (volatile int i=0; i<1000000; i++);
    }
    print_at(15, 17, "Scheduler initialized");

    print_at(15, 18, "Starting shell...");
    for (volatile int i=0; i<1000000; i++);
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

void print_string(const char* str) {
    while (*str) {
        putchar(*str++);
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
