#ifndef PARTY_H
#define PARTY_H

#include <stdint.h>

/*
 * PS2 Party Network System – LAN rooms, chat, session metadata.
 * Uses Ethernet adapter and transport layer.
 */

#define PARTY_ROOM_NAME_MAX  32
#define PARTY_MEMBER_MAX     8
#define PARTY_IP_STR_MAX     16
#define PARTY_CHAT_MSG_MAX   128

typedef struct {
    char ip[PARTY_IP_STR_MAX];
    uint8_t active;
    uint8_t reserved[3];
} party_member_t;

typedef struct {
    char name[PARTY_ROOM_NAME_MAX];
    party_member_t members[PARTY_MEMBER_MAX];
    int member_count;
    uint8_t is_host;
    uint8_t in_room;
} party_room_t;

/* Initialize party subsystem (call after transport_init). */
int party_init(void);

/* Create a new room (host). name optional, can be empty. */
int party_create(const char *name);

/* Invite / add peer by IP (e.g. "192.168.0.12"). */
int party_invite(const char *ip);

/* List available rooms (discovery) or current room members. */
int party_list(void);

/* Join room by IP; if ip is NULL, use discovery. */
int party_join(const char *ip);

/* Leave current room. */
void party_leave(void);

/* Send chat message (in-room only). */
int party_chat(const char *msg);

/* Get current room state for CLI. */
void party_get_room(party_room_t *out);

/* 1 = in room, 0 = not in room. */
int party_in_room(void);

/* Process any incoming party message (call from main loop or party_list). */
void party_poll(void);

#endif /* PARTY_H */
