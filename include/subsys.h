#ifndef SUBSYS_H
#define SUBSYS_H

#include <stdint.h>

typedef enum {
    SUBSYS_CORE = 0,
    SUBSYS_STORAGE,
    SUBSYS_NET,
    SUBSYS_INPUT,
    SUBSYS_AUDIO,
    SUBSYS_VIDEO,
    SUBSYS_IOP,
    SUBSYS_PARTY,
    SUBSYS_QUANTUM,
    SUBSYS_COUNT
} subsys_id_t;

#define CAP_NONE        0x00000000u
#define CAP_STORAGE     0x00000001u
#define CAP_NET_RAW     0x00000002u
#define CAP_NET_TCP     0x00000004u
#define CAP_INPUT       0x00000008u
#define CAP_AUDIO       0x00000010u
#define CAP_VIDEO       0x00000020u
#define CAP_IOP         0x00000040u
#define CAP_QUANTUM     0x00000080u
#define CAP_BT          0x00000100u

typedef struct {
    const char *name;
    subsys_id_t id;
    uint32_t capabilities;
    int (*init)(void);
    int (*status)(char *buf, int max);
    void (*tick)(void);
    void (*shutdown)(void);
} subsys_t;

void subsys_register_all(void);
void subsys_init_all(void);
void subsys_tick_all(void);
const subsys_t *subsys_get(subsys_id_t id);
int subsys_has_cap(subsys_id_t id, uint32_t cap);
int subsys_status_all(char *buf, int max);

#endif /* SUBSYS_H */
