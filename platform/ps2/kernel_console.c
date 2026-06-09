/* PS2 native console — scr_printf-based kernel I/O. */

#ifdef PLATFORM_PS2

#include "../../include/kernel.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <tamtypes.h>
#include <debug.h>

void show_enhanced_boot_splash(void) {
    scr_printf("\n");
    scr_printf("  ================================\n");
    scr_printf("   ASMOS PS2 Native v2.0\n");
    scr_printf("   FreeMCBoot Edition\n");
    scr_printf("  ================================\n");
    scr_printf("  Initializing components...\n");
    scr_printf("  [####################] 100%%\n");
    scr_printf("  Ready.\n\n");
}

void kprint_char(char c) {
    scr_printf("%c", c);
}

void kprint(const char *str) {
    if (str) scr_printf("%s", str);
}

void kprint_char_color(char c, uint8_t color) {
    (void)color;
    scr_printf("%c", c);
}

void kprint_color(const char *str, uint8_t color) {
    (void)color;
    if (str) scr_printf("%s", str);
}

void clear_screen(void) {
    scr_clear();
}

void print_decimal(int value) {
    scr_printf("%d", value);
}

void print_hex(unsigned int value) {
    scr_printf("0x%08X", value);
}

void print_string(const char *str) {
    if (str) scr_printf("%s", str);
}

void kprintf(const char *format, ...) {
    char buf[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    scr_printf("%s", buf);
}

int ksstrcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int ksscanf(const char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsscanf(str, format, args);
    va_end(args);
    return n;
}

#endif /* PLATFORM_PS2 */
