#include "quantum.h"
#include "kernel.h"

#define MODES 3
static uint8_t mode_gain[MODES] = { 255, 255, 255 };
static const char *mode_names[MODES] = { "bass", "melody", "ambience" };

int quantum_measure_mode(int mode) {
    if (mode < 0 || mode >= MODES) return -1;
    mode_gain[mode] = 0;
    kprintf("measure: %s muted\n", mode_names[mode]);
    return 0;
}

int quantum_interfere_modes(int a, int b) {
    if (a < 0 || a >= MODES || b < 0 || b >= MODES) return -1;
    uint8_t avg = (uint8_t)((mode_gain[a] + mode_gain[b]) / 2);
    mode_gain[a] = mode_gain[b] = avg;
    kprintf("interfere: %s + %s\n", mode_names[a], mode_names[b]);
    return 0;
}
