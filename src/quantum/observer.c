#include "quantum.h"
#include "memory_budget.h"
#include "kernel.h"

static quantum_state_t qstate;

void quantum_init(void) {
    int i;
    for (i = 0; i < QUANTUM_PROFILE_MAX; i++) {
        qstate.profiles[i].name[0] = '\0';
        qstate.profiles[i].weight = 0;
        qstate.profiles[i].collapsed = 0;
        qstate.profiles[i].ram_cost_kb = 0;
    }
    qstate.profile_count = 0;
    qstate.active[0] = '\0';
    qstate.coherence = 200;
}

uint8_t quantum_coherence(void) {
    return qstate.coherence;
}

int quantum_observe(const char *subsys) {
    if (!subsys) return -1;
    memory_budget_alloc(BUDGET_APP, 256 * 1024);
    (void)subsys;
    kprintf("observe: %s collapsed into RAM\n", subsys);
    if (qstate.coherence > 10) qstate.coherence -= 5;
    return 0;
}

int quantum_collapse(const char *profile) {
    int i;
    if (!profile) return -1;
    for (i = 0; i < qstate.profile_count; i++) {
        if (qstate.profiles[i].name[0] == profile[0]) {
            qstate.profiles[i].collapsed = 1;
            int j = 0;
            while (profile[j] && j < QUANTUM_NAME_MAX - 1) {
                qstate.active[j] = profile[j];
                j++;
            }
            qstate.active[j] = '\0';
            kprintf("collapse: profile %s active\n", profile);
            return 0;
        }
    }
    return -1;
}

int quantum_superpose(const char *a, const char *b, uint8_t weight) {
    if (!a || !b || qstate.profile_count >= QUANTUM_PROFILE_MAX) return -1;
    quantum_profile_t *p = &qstate.profiles[qstate.profile_count++];
    int i = 0;
    while (a[i] && i < QUANTUM_NAME_MAX - 1) { p->name[i] = a[i]; i++; }
    p->name[i] = '+';
    if (i < QUANTUM_NAME_MAX - 1) p->name[++i] = '\0';
    p->weight = weight;
    p->collapsed = 0;
    (void)b;
    kprintf("superpose: %s | %s weight=%u\n", a, b, (unsigned)weight);
    return 0;
}

int quantum_status(char *buf, int max) {
    if (!buf || max < 16) return -1;
    int pos = 0;
    const char *prefix = "coh=";
    while (*prefix && pos < max - 1) buf[pos++] = *prefix++;
    if (qstate.coherence >= 100 && pos < max - 1) buf[pos++] = '1';
    if (pos < max - 1) buf[pos++] = '0' + (qstate.coherence / 10) % 10;
    if (pos < max - 1) buf[pos++] = '0' + qstate.coherence % 10;
    buf[pos] = '\0';
    return 0;
}

extern int quantum_phase_sync_tick(void);

void quantum_tick(void) {
    if (qstate.coherence < 255) qstate.coherence++;
    quantum_phase_sync_tick();
}
