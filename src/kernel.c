#include "msp.h"
#include "shell.h"
#include "memory_manager.h"
#include "scheduler.h"
#include "fs.h"
#include "kernel.h"
#include "platform.h"
#include "net.h"
#include "game.h"
#include "game_history.h"
#include "music.h"
#include "controller_remap.h"
#include "save_manager.h"
#include "dashboard.h"
#include "storage.h"
#include "pause_engine.h"
#include "memory_budget.h"
#include "subsys.h"
#include <stdarg.h>
#include <stdint.h>
#include "arch_x86.h"
#ifndef PLATFORM_PS2
#include "interrupts.h"
#endif

#ifndef PLATFORM_PS2
// VGA text buffer starts at 0xB8000
volatile uint16_t* vga_buffer = (uint16_t*)VGA_ADDRESS;
int cursor_row = 0;
int cursor_col = 0;
#endif

// PS2 system information
static struct {
    uint32_t total_memory;
    uint8_t ps2_detected;
    uint8_t dualshock_support;
    uint8_t network_support;
} ps2_info = {0};

// Enhanced kernel entry point with PS2 optimizations
void kernel_main(void) {
    plat_init();
#ifndef PLATFORM_PS2
    /* Install the IDT and remap the 8259s before anything can raise an
     * interrupt. SeaBIOS leaves IDTR pointing at its real-mode IVT, whose
     * entries are 16-bit far pointers rather than 32-bit gates — with IF
     * set and no LIDT ever executed, the first interrupt or exception
     * vectors into that garbage, double-faults, and resets the machine
     * before the shell ever runs. */
    interrupt_init();
#endif
    subsys_register_all();
    kprint("ASMOS Kernel v3.0 - Physical Console Edition\n");
#ifndef PLATFORM_PS2
    disable_interrupts_asm();
#endif

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

    kprint("Initializing scheduler and filesystem...\n");
    init_scheduler();
    plat_fs_init();
    
    memory_budget_init();
    kprint("Initializing subsystems...\n");
    subsys_init_all();
    net_init();
    
    storage_init();
    pause_engine_init();
    kprint("Initializing enhanced shell...\n");
    init_shell();
    kprint("Initializing game system...\n");
    init_game_system();
    game_history_init();
    music_init();
    controller_remap_init();
    save_manager_init();
    dashboard_init();

    kprint("Showing enhanced boot splash...\n");
    show_enhanced_boot_splash(); 
    
#ifndef PLATFORM_PS2
    enable_interrupts_asm();
#endif
    kprint("ASMOS Kernel v2.0 - Ready!\n");
    
    start_shell();
    // Catch
    halt_system();
}

// System halt function
extern void system_reboot(void);

void halt_system(void) {
    kprint("System halted.\n");
    system_reboot();
}

// Get system memory function
uint32_t get_system_memory(void) {
    return ps2_info.total_memory;
}

#ifndef PLATFORM_PS2

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

void clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = ((uint16_t)DEFAULT_COLOR << 8) | ' ';
    }
    cursor_row = 0;
    cursor_col = 0;
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

/* Render an unsigned value into buf (NUL-terminated), zero-padded to
 * 'min_digits'. Returns the length written. */
static int fmt_u32(char *buf, unsigned int value, unsigned int min_digits) {
    char tmp[12];
    int n = 0;
    do {
        tmp[n++] = (char)('0' + (value % 10));
        value /= 10;
    } while (value);
    int len = 0;
    for (int i = n; i < (int)min_digits; i++)
        buf[len++] = '0';
    for (int i = n - 1; i >= 0; i--)
        buf[len++] = tmp[i];
    buf[len] = '\0';
    return len;
}

/* kprintf understands %s %c %d %i %u %x %X %p %% plus a flag/width pair:
 * '-' left-justify, '0' zero-pad, and a decimal width. Length modifiers
 * (l/h/z) are accepted and ignored — this kernel is 32-bit only and
 * formats through unsigned int internally. There is no libc here to lean
 * on, so the padding is done by hand; without it every "%-10s" in the
 * shell printed a literal "?" instead of a padded field. */
void kprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    for (const char* ptr = format; *ptr != '\0'; ptr++) {
        if (*ptr != '%') {
            putchar(*ptr);
            continue;
        }
        ptr++;

        int left = 0, zero = 0, width = 0;
        if (*ptr == '-') { left = 1; ptr++; }
        if (*ptr == '0') { zero = 1; ptr++; }
        while (*ptr >= '0' && *ptr <= '9') {
            width = width * 10 + (*ptr - '0');
            ptr++;
        }
        while (*ptr == 'l' || *ptr == 'h' || *ptr == 'z') ptr++;

        char pad = (zero && !left) ? '0' : ' ';

        switch (*ptr) {
            case 's': {
                const char *str = va_arg(args, const char *);
                if (!str) str = "(null)";
                int len = 0;
                while (str[len]) len++;
                for (int i = len; i < width && !left; i++) putchar(' ');
                for (int i = 0; i < len; i++) putchar(str[i]);
                for (int i = len; i < width && left; i++) putchar(' ');
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                for (int i = 1; i < width && !left; i++) putchar(' ');
                putchar(c);
                for (int i = 1; i < width && left; i++) putchar(' ');
                break;
            }
            case 'd':
            case 'i': {
                int val = va_arg(args, int);
                unsigned int mag = (val < 0) ? (unsigned int)(-(long)val) : (unsigned int)val;
                char buf[16];
                int len = fmt_u32(buf, mag, 0);
                int sign = (val < 0) ? 1 : 0;
                for (int i = len + sign; i < width && !left; i++) putchar(pad);
                if (sign) putchar('-');
                print_string(buf);
                for (int i = len + sign; i < width && left; i++) putchar(' ');
                break;
            }
            case 'u': {
                char buf[16];
                int len = fmt_u32(buf, va_arg(args, unsigned int), (unsigned int)zero ? (unsigned int)width : 0);
                for (int i = len; i < width && !left; i++) putchar(' ');
                print_string(buf);
                for (int i = len; i < width && left; i++) putchar(' ');
                break;
            }
            case 'x':
            case 'X':
            case 'p': {
                const char *digits = (*ptr == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
                unsigned int val = va_arg(args, unsigned int);
                char buf[12];
                for (int i = 0; i < 8; i++)
                    buf[i] = digits[(val >> ((7 - i) * 4)) & 0xF];
                buf[8] = '\0';
                for (int i = 8; i < width && !left; i++) putchar(pad);
                print_string(buf);
                for (int i = 8; i < width && left; i++) putchar(' ');
                break;
            }
            case '%':
                putchar('%');
                break;
            default:
                putchar('%');
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

#endif /* !PLATFORM_PS2 */
