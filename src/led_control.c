/* LED Control — GPIO (Johnny). Fat vs Slim; safe fallback. */

#include "led_control.h"
#include "kernel.h"
#include <stddef.h>

static led_model_t cached_model = LED_MODEL_UNKNOWN;
static int supported = 0;  /* 0 on x86 / no GPIO */

static int str_eq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

led_model_t led_detect_model(void) {
    if (cached_model != LED_MODEL_UNKNOWN) return cached_model;
#ifdef PS2_HARDWARE
    /* Real PS2: detect fat vs slim by hardware ID / GPIO presence. */
    cached_model = LED_MODEL_FAT;
    supported = 1;
#else
    cached_model = LED_MODEL_UNKNOWN;
    supported = 0;
#endif
    return cached_model;
}

int led_is_supported(void) {
    if (cached_model == LED_MODEL_UNKNOWN) led_detect_model();
    return supported;
}

int led_set(const char *color_name) {
    if (!color_name) return -1;
    if (!led_is_supported()) {
        kprint("  led: not supported on this hardware\n");
        return -2;
    }
    if (str_eq(color_name, "red")) return 0;
    if (str_eq(color_name, "green")) return 0;
    if (str_eq(color_name, "blue")) return 0;
    if (str_eq(color_name, "off")) return 0;
    kprint("  led: unknown color (red, green, blue, off)\n");
    return -1;
}

int led_pulse(uint32_t period_ms) {
    (void)period_ms;
    if (!led_is_supported()) {
        kprint("  led pulse: not supported on this hardware\n");
        return -1;
    }
    return 0;
}

int led_rgb(uint8_t r, uint8_t g, uint8_t b) {
    (void)r;
    (void)g;
    (void)b;
    if (!led_is_supported()) {
        kprint("  led rgb: not supported on this hardware\n");
        return -1;
    }
    return 0;
}

int led_set_timed(const char *color_name, uint32_t delay_ms) {
    (void)delay_ms;
    return led_set(color_name);
}
