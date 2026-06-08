#include "quantum.h"
#include "controller_remap.h"
#include "kernel.h"

static int entangled_a = -1;
static int entangled_b = -1;

int quantum_entangle_input(int pad_a, int pad_b) {
    entangled_a = pad_a;
    entangled_b = pad_b;
    kprintf("entangle: pad%d <-> pad%d\n", pad_a, pad_b);
    return 0;
}

int quantum_decohere(int pad) {
    if (entangled_a == pad || entangled_b == pad) {
        entangled_a = entangled_b = -1;
        kprintf("decohere: pad%d\n", pad);
        return 0;
    }
    return -1;
}

void quantum_entangle_tick(void) {
    if (entangled_a < 0 || entangled_b < 0) return;
    (void)entangled_a;
}
