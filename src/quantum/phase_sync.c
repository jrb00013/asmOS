#include "quantum.h"
#include "transport.h"

static uint8_t phase[8];
static uint8_t omega[8];
static uint8_t coupling = 128;
static int node_count = 1;

void quantum_set_coupling(uint8_t k) {
    coupling = k;
}

int quantum_phase_sync_tick(void) {
    int i, j;
    for (i = 0; i < node_count; i++) {
        int sum = 0;
        for (j = 0; j < node_count; j++) {
            if (i == j) continue;
            int d = (int)phase[j] - (int)phase[i];
            if (d > 128) d -= 256;
            if (d < -128) d += 256;
            sum += d;
        }
        int delta = (coupling * sum) / (256 * node_count);
        phase[i] = (uint8_t)((int)phase[i] + omega[i] + delta);
    }
    transport_tick();
    return 0;
}
