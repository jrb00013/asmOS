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


#endif // KERNEL_H