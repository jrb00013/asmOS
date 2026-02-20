#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

/* LED Control — GPIO (Johnny). led set, led pulse, led rgb.
 * Fat vs Slim detection; safe fallback if unsupported. */

typedef enum {
    LED_MODEL_UNKNOWN = 0,
    LED_MODEL_FAT,
    LED_MODEL_SLIM,
} led_model_t;

typedef enum {
    LED_COLOR_OFF = 0,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_RGB,
} led_color_mode_t;

/* Detect console model (fat vs slim) for LED GPIO. */
led_model_t led_detect_model(void);

/* Set solid color: red, green, blue, or off. */
int led_set(const char *color_name);

/* Pulse: cycle or breathe. period_ms 0 = default. */
int led_pulse(uint32_t period_ms);

/* RGB: r,g,b in 0–255. */
int led_rgb(uint8_t r, uint8_t g, uint8_t b);

/* One-shot: set then restore after delay_ms. */
int led_set_timed(const char *color_name, uint32_t delay_ms);

/* Check if LED is supported on this hardware. */
int led_is_supported(void);

#endif /* LED_CONTROL_H */
