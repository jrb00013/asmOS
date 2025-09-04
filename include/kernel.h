#ifndef KERNEL_H
#define KERNEL_H

#include "ps2_hardware.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT   25
#define VGA_ADDRESS     0xB8000
#define DEFAULT_COLOR   0x07

#define NULL ((void*)0)

// Kernel functions
void kprint(const char *str);
void kprint_char(char c);
void kprintf(const char *format, ...);
void print_hex(unsigned int value);
void print_decimal(int value);
void putchar(char c);
void print_string(const char* str);
int ksstrcmp(const char *s1, const char *s2);
int ksscanf(const char *str, const char *format, ...);

// Enhanced boot splash
void show_enhanced_boot_splash(void);

// Color printing
void kprint_char_color(char c, uint8_t color);
void kprint_color(const char* str, uint8_t color);

// PS2-specific functions
void halt_system(void);
uint32_t get_system_memory(void);

// Utility functions
static void draw_box(int x, int y, int w, int h, uint8_t color);
static void print_at(int x, int y, const char* str);
static void draw_progress_bar(int x, int y, int width, int percent);

#endif // KERNEL_H