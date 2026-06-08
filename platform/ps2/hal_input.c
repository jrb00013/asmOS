#include "platform.h"
#include <pad.h>
#include <stdint.h>

static int pad_ready;

int plat_keyboard_scancode(void) {
    return 0;
}

int plat_keyboard_has_key(void) {
    return 0;
}

int plat_controller_read(int port, plat_controller_state_t *out) {
    if (!out || port != 0) return -1;
    int padStatus = padGetState(0, port);
    if (padStatus != PAD_STATE_STABLE && padStatus != PAD_STATE_FINDCTP1) {
        out->present = 0;
        return -1;
    }
    padButtonStatus buttons;
    if (padRead(0, port, &buttons) != 0) {
        out->present = 0;
        return -1;
    }
    out->present = 1;
    out->buttons = buttons.btns;
    out->lx = (int8_t)buttons.ljoy_h;
    out->ly = (int8_t)buttons.ljoy_v;
    out->rx = (int8_t)buttons.rjoy_h;
    out->ry = (int8_t)buttons.rjoy_v;
    pad_ready = 1;
    return 0;
}
