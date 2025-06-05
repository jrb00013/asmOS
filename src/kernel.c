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
    
    // Load additional kernel components from disk
    load_kernel_from_disk();
    
    // Initialize system components
    init_scheduler();
    init_fat12();  // Initialize filesystem
    init_shell();
    
    // Enable interrupts and start system
    enable_interrupts_asm();
    kprint("PS2 x86 OS Kernel v1.0\n");
    
    // Start the shell
    start_shell();
    
    // Should never reach here
    halt_system();
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
