#ifndef NETWORK_CONTROLLER_H
#define NETWORK_CONTROLLER_H

#include <stdint.h>

/* Cross-controller workaround without Bluetooth: virtual controller over network.
 * A phone/app sends button/axis state via UDP; we expose it as "port 2" or
 * a virtual pad. So "cross-controller" works without any BT dongle. */

#define NETCTRL_PORT         31337   /* UDP port for controller packets */
#define NETCTRL_MAGIC        0x43543250  /* "CT2P" */

typedef struct {
    uint32_t magic;
    uint16_t buttons;       /* same layout as DS2 */
    int8_t lx, ly, rx, ry;  /* -128..127 */
    uint8_t lt, rt;         /* 0..255 */
} network_controller_packet_t;

/* Start listening for UDP controller packets. */
int network_controller_start(void);

/* Stop listener. */
void network_controller_stop(void);

/* Get last received state (for controller layer). */
void network_controller_get_state(uint16_t *buttons, int8_t *lx, int8_t *ly, int8_t *rx, int8_t *ry);

/* Poll: read one packet if available. Call from main loop or network task. */
int network_controller_poll(void);

#endif /* NETWORK_CONTROLLER_H */
