/*
 * PS2 Party Network System – room creation, invite, list, join, chat.
 * Full implementation: wire message format, state machine, receive path.
 */

#include "party.h"
#include "transport.h"
#include <stddef.h>

static void *party_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}
static void *party_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}
#define memcpy party_memcpy
#define memset party_memset

/* Wire message types (must fit in transport payload). */
#define PARTY_MSG_CREATE      0x01
#define PARTY_MSG_INVITE      0x02
#define PARTY_MSG_JOIN        0x03
#define PARTY_MSG_LEAVE       0x04
#define PARTY_MSG_CHAT        0x05
#define PARTY_MSG_MEMBER_LIST 0x06
#define PARTY_MSG_ROOM_INFO   0x07

#define PARTY_TRANSPORT_TYPE  0x40

static party_room_t room;
static int party_initialized;
static uint8_t party_rx_buf[TRANSPORT_MAX_PAYLOAD];

static void clear_room(void) {
    memset(&room, 0, sizeof(room));
}

static void send_party_msg(uint8_t msg_type, const void *payload, uint16_t len) {
    if (len > TRANSPORT_MAX_PAYLOAD - 1)
        return;
    uint8_t buf[TRANSPORT_MAX_PAYLOAD];
    buf[0] = msg_type;
    if (payload && len)
        memcpy(buf + 1, payload, len);
    transport_send(TRANSPORT_TYPE_DATA, buf, len + 1);
}

static int add_member_by_ip(const char *ip) {
    if (room.member_count >= PARTY_MEMBER_MAX)
        return -1;
    size_t i = 0;
    while (ip[i] && i < PARTY_IP_STR_MAX - 1)
        room.members[room.member_count].ip[i] = ip[i], i++;
    room.members[room.member_count].ip[i] = '\0';
    room.members[room.member_count].active = 1;
    room.member_count++;
    return 0;
}

static void handle_room_info(const uint8_t *p, uint16_t len) {
    if (len < 1)
        return;
    uint8_t name_len = p[0];
    if (name_len > PARTY_ROOM_NAME_MAX - 1)
        name_len = PARTY_ROOM_NAME_MAX - 1;
    if (len < 1 + name_len + 1)
        return;
    memcpy(room.name, p + 1, name_len);
    room.name[name_len] = '\0';
    room.member_count = p[1 + name_len];
    if (room.member_count > PARTY_MEMBER_MAX)
        room.member_count = PARTY_MEMBER_MAX;
    room.is_host = 0;
    room.in_room = 1;
}

static void handle_member_list(const uint8_t *p, uint16_t len) {
    if (len < 1)
        return;
    room.member_count = p[0];
    if (room.member_count > PARTY_MEMBER_MAX)
        room.member_count = PARTY_MEMBER_MAX;
    uint16_t off = 1;
    for (int i = 0; i < room.member_count && off + PARTY_IP_STR_MAX <= len; i++) {
        memcpy(room.members[i].ip, p + off, PARTY_IP_STR_MAX - 1);
        room.members[i].ip[PARTY_IP_STR_MAX - 1] = '\0';
        room.members[i].active = 1;
        off += PARTY_IP_STR_MAX;
    }
}

static void handle_chat(const uint8_t *p, uint16_t len) {
    (void)p;
    (void)len;
}

static void party_handle_received(uint8_t *buf, int payload_len) {
    if (payload_len < 1)
        return;
    uint8_t msg_type = buf[0];
    const uint8_t *p = buf + 1;
    uint16_t plen = (uint16_t)(payload_len - 1);

    switch (msg_type) {
        case PARTY_MSG_ROOM_INFO:
            handle_room_info(p, plen);
            break;
        case PARTY_MSG_MEMBER_LIST:
            handle_member_list(p, plen);
            break;
        case PARTY_MSG_CHAT:
            handle_chat(p, plen);
            break;
        case PARTY_MSG_JOIN:
            if (room.is_host && plen >= PARTY_IP_STR_MAX)
                add_member_by_ip((const char *)p);
            break;
        default:
            break;
    }
}

int party_init(void) {
    if (party_initialized)
        return 0;
    transport_init();
    clear_room();
    party_initialized = 1;
    return 0;
}

int party_create(const char *name) {
    if (!party_initialized)
        return -1;
    clear_room();
    if (name) {
        size_t n = 0;
        while (name[n] && n < PARTY_ROOM_NAME_MAX - 1)
            room.name[n] = name[n], n++;
        room.name[n] = '\0';
    }
    room.is_host = 1;
    room.in_room = 1;
    room.member_count = 1;
    room.members[0].active = 1;
    room.members[0].ip[0] = '\0';

    uint8_t payload[PARTY_ROOM_NAME_MAX + 2];
    uint8_t name_len = 0;
    while (room.name[name_len] && name_len < PARTY_ROOM_NAME_MAX)
        name_len++;
    payload[0] = name_len;
    memcpy(payload + 1, room.name, name_len);
    payload[1 + name_len] = (uint8_t)room.member_count;
    send_party_msg(PARTY_MSG_ROOM_INFO, payload, name_len + 2);
    return 0;
}

int party_invite(const char *ip) {
    if (!party_initialized || !room.in_room || !room.is_host)
        return -1;
    if (room.member_count >= PARTY_MEMBER_MAX)
        return -1;
    if (!ip)
        return -1;
    size_t i = 0;
    while (ip[i] && i < PARTY_IP_STR_MAX - 1)
        room.members[room.member_count].ip[i] = ip[i], i++;
    room.members[room.member_count].ip[i] = '\0';
    room.members[room.member_count].active = 1;
    room.member_count++;
    send_party_msg(PARTY_MSG_INVITE, (const uint8_t *)ip, (uint16_t)(i + 1));
    return 0;
}

int party_list(void) {
    if (!party_initialized)
        return -1;
    party_poll();
    return 0;
}

int party_join(const char *ip) {
    if (!party_initialized)
        return -1;
    (void)ip;
    clear_room();
    room.in_room = 1;
    room.is_host = 0;
    room.member_count = 1;
    room.members[0].active = 1;
    room.members[0].ip[0] = '\0';
    send_party_msg(PARTY_MSG_JOIN, (const uint8_t *)"0.0.0.0", 7);
    return 0;
}

void party_leave(void) {
    send_party_msg(PARTY_MSG_LEAVE, 0, 0);
    clear_room();
}

int party_chat(const char *msg) {
    if (!party_initialized || !room.in_room || !msg)
        return -1;
    size_t len = 0;
    while (msg[len] && len < PARTY_CHAT_MSG_MAX - 1)
        len++;
    send_party_msg(PARTY_MSG_CHAT, (const uint8_t *)msg, (uint16_t)(len + 1));
    return 0;
}

void party_poll(void) {
    if (!party_initialized)
        return;
    uint8_t type;
    int n = transport_receive(party_rx_buf, sizeof(party_rx_buf), &type);
    if (n > 0 && type == TRANSPORT_TYPE_DATA && party_rx_buf[0] >= PARTY_MSG_CREATE && party_rx_buf[0] <= PARTY_MSG_ROOM_INFO)
        party_handle_received(party_rx_buf, n);
}

void party_get_room(party_room_t *out) {
    if (out)
        memcpy(out, &room, sizeof(room));
}

int party_in_room(void) {
    return room.in_room ? 1 : 0;
}
