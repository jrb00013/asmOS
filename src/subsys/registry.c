#include "subsys.h"
#include "platform.h"
#include "quantum.h"
#include "memory_budget.h"
#include "party.h"
#include "transport.h"
#include "kernel.h"

static int core_init(void) { return 0; }
static int core_status(char *buf, int max) {
    if (!buf || max < 2) return -1;
    buf[0] = 'o'; buf[1] = 'k'; buf[2] = '\0';
    return 0;
}

static int storage_init(void) { return plat_fs_init(); }
static int storage_status(char *buf, int max) {
    if (!buf || max < 8) return -1;
    buf[0] = plat_fs_validate() == 0 ? 'm' : 'e';
    buf[1] = 'o'; buf[2] = 'u'; buf[3] = 'n'; buf[4] = 't'; buf[5] = '\0';
    return 0;
}

static int net_init_sub(void) { return plat_net_init(); }
static int net_status(char *buf, int max) {
    plat_net_info_t ni;
    plat_net_get_info(&ni);
    int i = 0;
    const char *s = ni.linked ? "up" : "down";
    while (s[i] && i < max - 1) { buf[i] = s[i]; i++; }
    buf[i] = '\0';
    return 0;
}
static void net_tick(void) { transport_tick(); }

static int input_init(void) { return 0; }
static int input_status(char *buf, int max) {
    if (!buf || max < 4) return -1;
    buf[0] = 'r'; buf[1] = 'e'; buf[2] = 'a'; buf[3] = 'd'; buf[4] = '\0';
    return 0;
}

static int audio_init(void) { return 0; }
static int audio_status(char *buf, int max) {
    if (!buf || max < 4) return -1;
    buf[0] = 's'; buf[1] = 'p'; buf[2] = 'u'; buf[3] = '\0';
    return 0;
}

static int video_init(void) { plat_video_mode_text(); return 0; }
static int video_status(char *buf, int max) {
    if (!buf || max < 4) return -1;
    buf[0] = 'v'; buf[1] = 'g'; buf[2] = 'a'; buf[3] = '\0';
    return 0;
}

static int iop_init_sub(void);
static int iop_status(char *buf, int max);

static int party_init_sub(void) { return 0; }
static void party_tick(void) { party_poll(); }
static int party_status(char *buf, int max) {
    if (!buf || max < 4) return -1;
    buf[0] = party_in_room() ? 'i' : 'o';
    buf[1] = 'p'; buf[2] = 'e'; buf[3] = 'n'; buf[4] = '\0';
    return 0;
}

static int quantum_init_sub(void) { quantum_init(); return 0; }
static int quantum_status_sub(char *buf, int max) { return quantum_status(buf, max); }

#ifndef PLATFORM_PS2
static int iop_init_sub(void) { return 0; }
static int iop_status(char *buf, int max) {
    if (!buf || max < 6) return -1;
    buf[0] = 's'; buf[1] = 'i'; buf[2] = 'm'; buf[3] = '\0';
    return 0;
}
#else
extern int ps2_iop_init(void);
extern int ps2_iop_status(char *buf, int max);
static int iop_init_sub(void) { return ps2_iop_init(); }
static int iop_status(char *buf, int max) { return ps2_iop_status(buf, max); }
#endif

static subsys_t subsystems[SUBSYS_COUNT] = {
    { "core",    SUBSYS_CORE,    CAP_NONE,    core_init,    core_status,    NULL,         NULL },
    { "storage", SUBSYS_STORAGE, CAP_STORAGE, storage_init, storage_status, NULL,         NULL },
    { "net",     SUBSYS_NET,     CAP_NET_RAW, net_init_sub, net_status,     net_tick,     NULL },
    { "input",   SUBSYS_INPUT,   CAP_INPUT,   input_init,   input_status,   NULL,         NULL },
    { "audio",   SUBSYS_AUDIO,   CAP_AUDIO,   audio_init,   audio_status,   NULL,         NULL },
    { "video",   SUBSYS_VIDEO,   CAP_VIDEO,   video_init,   video_status,   NULL,         NULL },
    { "iop",     SUBSYS_IOP,     CAP_IOP,     iop_init_sub, iop_status,     NULL,         NULL },
    { "party",   SUBSYS_PARTY,   CAP_NET_RAW, party_init_sub, party_status, party_tick, NULL },
    { "quantum", SUBSYS_QUANTUM, CAP_QUANTUM, quantum_init_sub, quantum_status_sub, (void (*)(void))quantum_tick, NULL },
};

void subsys_register_all(void) { /* static table */ }

void subsys_init_all(void) {
    int i;
    for (i = 0; i < SUBSYS_COUNT; i++)
        if (subsystems[i].init) subsystems[i].init();
}

void subsys_tick_all(void) {
    int i;
    for (i = 0; i < SUBSYS_COUNT; i++)
        if (subsystems[i].tick) subsystems[i].tick();
}

const subsys_t *subsys_get(subsys_id_t id) {
    if (id < 0 || id >= SUBSYS_COUNT) return 0;
    return &subsystems[id];
}

int subsys_has_cap(subsys_id_t id, uint32_t cap) {
    const subsys_t *s = subsys_get(id);
    if (!s) return 0;
    return (s->capabilities & cap) != 0;
}

int subsys_status_all(char *buf, int max) {
    int i, pos = 0;
    for (i = 0; i < SUBSYS_COUNT && pos < max - 1; i++) {
        char tmp[32];
        if (subsystems[i].status && subsystems[i].status(tmp, sizeof(tmp)) == 0) {
            int j = 0;
            while (subsystems[i].name[j] && pos < max - 1) buf[pos++] = subsystems[i].name[j++];
            if (pos < max - 1) buf[pos++] = '=';
            j = 0;
            while (tmp[j] && pos < max - 1) buf[pos++] = tmp[j++];
            if (pos < max - 1) buf[pos++] = ' ';
        }
    }
    buf[pos] = '\0';
    return 0;
}
