#ifndef KERNEL_H
#define KERNEL_H

#define VGA_WIDTH   80
#define VGA_HEIGHT   25
#define VGA_ADDRESS     0xB8000
#define DEFAULT_COLOR   0x07

#define NULL ((void*)0)

void kprint(const char *str);
void kprint_char(char c);
void kprintf(const char *format, ...);
void print_hex(unsigned int value);
void print_decimal(int value);
void putchar(char c);
void print_string(const char* str);
int ksstrcmp(const char *s1, const char *s2);
int ksscanf(const char *str, const char *format, ...);
static void draw_box(int x, int y, int w, int h, uint8_t color);
static void print_at(int x, int y, const char* str);
static void draw_progress_bar(int x, int y, int width, int percent);
void show_boot_splash(void);
#endif // KERNEL_H