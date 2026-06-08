#ifndef QUANTUM_H
#define QUANTUM_H

#include <stdint.h>

#define QUANTUM_PROFILE_MAX 4
#define QUANTUM_NAME_MAX    16

typedef struct {
    char name[QUANTUM_NAME_MAX];
    uint8_t weight;       /* 0-255 amplitude */
    uint8_t collapsed;    /* 1 if observed */
    uint32_t ram_cost_kb;
} quantum_profile_t;

typedef struct {
    quantum_profile_t profiles[QUANTUM_PROFILE_MAX];
    int profile_count;
    char active[QUANTUM_NAME_MAX];
    uint8_t coherence;    /* 0-255 system coherence */
} quantum_state_t;

void quantum_init(void);
void quantum_tick(void);
int  quantum_observe(const char *subsys);
int  quantum_collapse(const char *profile);
int  quantum_superpose(const char *a, const char *b, uint8_t weight);
int  quantum_entangle_input(int pad_a, int pad_b);
int  quantum_decohere(int pad);
uint8_t quantum_coherence(void);
int  quantum_phase_sync_tick(void);
void quantum_set_coupling(uint8_t k);
int  quantum_load_config_wf(const void *data, uint32_t len);
int  quantum_status(char *buf, int max);

#endif /* QUANTUM_H */
