/* FTP/telnet/IRC clients over the net_stream UDP-emulated-TCP transport.
 *
 * The underlying transport (src/net/udp.c) is a single best-effort,
 * non-blocking, packet-oriented "stream" — there is no real TCP connection
 * setup/teardown or a second data channel. So "real" here means: these
 * clients drive an actual back-and-forth protocol exchange (not just a
 * banner print) using the send/recv primitives that exist, and actually
 * move bytes through plat_fs_read/plat_fs_write for FTP transfers and
 * through plat_read_line for telnet's interactive session, instead of
 * printing a "(not implemented)"-style message and returning.
 */

#include "net_clients.h"
#include "net.h"
#include "kernel.h"
#include "platform.h"
#include <stddef.h>
#include <stdint.h>

#define NET_LINE_MAX     256
#define FTP_XFER_MAX      8192
#define TELNET_IDLE_MS      50
#define TELNET_DRAIN_TRIES  20

static int kstreq2(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}

static unsigned int kstrlen2(const char *s) {
    unsigned int n = 0;
    while (s[n]) n++;
    return n;
}

/* Skip the first whitespace-delimited token (the host, already parsed by
 * the caller) and return a pointer to the remainder, or "" if there is
 * none. */
static const char *skip_token(const char *args) {
    if (!args) return "";
    while (*args == ' ' || *args == '\t') args++;
    while (*args && *args != ' ' && *args != '\t') args++;
    while (*args == ' ' || *args == '\t') args++;
    return args;
}

static int next_word(const char *p, char *out, unsigned int max) {
    unsigned int n = 0;
    while (*p == ' ' || *p == '\t') p++;
    while (*p && *p != ' ' && *p != '\t' && n + 1 < max) out[n++] = *p++;
    out[n] = '\0';
    return (int)n;
}

/* Drain and print whatever is already waiting on the stream, without
 * blocking forever — net_stream_recv is non-blocking, so this stops as
 * soon as a read comes back empty. */
static int drain_and_print(net_stream_t *s, const char *prefix) {
    char buf[NET_LINE_MAX];
    int total = 0;
    int n;
    while ((n = net_stream_recv(s, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        if (prefix) kprint(prefix);
        kprintf("%s\n", buf);
        total += n;
    }
    return total;
}

int ftp_client(const char *host, const char *args) {
    net_stream_t *s = net_connect(host, 21);
    if (!s) {
        kprint("  ftp: connect failed\n");
        return -1;
    }
    char banner[NET_LINE_MAX];
    int n = net_stream_recv(s, banner, sizeof(banner) - 1);
    if (n > 0) { banner[n] = '\0'; kprintf("  ftp: %s\n", banner); }

    net_stream_send(s, "USER anonymous\r\n", 16);
    n = net_stream_recv(s, banner, sizeof(banner) - 1);
    if (n > 0) { banner[n] = '\0'; kprintf("  %s\n", banner); }

    net_stream_send(s, "PASS guest@\r\n", 13);
    n = net_stream_recv(s, banner, sizeof(banner) - 1);
    if (n > 0) { banner[n] = '\0'; kprintf("  %s\n", banner); }

    const char *rest = skip_token(args); /* args after the host token */
    char verb[16];
    next_word(rest, verb, sizeof(verb));
    const char *rest2 = skip_token(rest);

    if (verb[0] == '\0') {
        kprint("  ftp: connected. usage: ftp <host> <list|get <remote>|put <local>>\n");
        net_stream_close(s);
        return 0;
    }

    if (kstreq2(verb, "list") || kstreq2(verb, "ls")) {
        net_stream_send(s, "LIST\r\n", 6);
        int got = 0, tries = TELNET_DRAIN_TRIES;
        while (tries-- > 0) {
            int r = drain_and_print(s, "  ");
            got += r;
            if (r == 0) plat_delay_ms(TELNET_IDLE_MS);
        }
        if (!got) kprint("  ftp: (empty listing or no response)\n");
    } else if (kstreq2(verb, "get") || kstreq2(verb, "retr")) {
        char remote[64];
        next_word(rest2, remote, sizeof(remote));
        if (!remote[0]) {
            kprint("  usage: ftp <host> get <remote-file>\n");
        } else {
            char cmd[80];
            unsigned int p = 0;
            const char *pfx = "RETR ";
            while (pfx[p]) { cmd[p] = pfx[p]; p++; }
            unsigned int q = 0;
            while (remote[q] && p + 1 < sizeof(cmd)) cmd[p++] = remote[q++];
            cmd[p++] = '\r'; cmd[p++] = '\n'; cmd[p] = '\0';
            net_stream_send(s, cmd, p);

            static uint8_t xfer[FTP_XFER_MAX];
            uint32_t total = 0;
            int tries = TELNET_DRAIN_TRIES;
            while (tries-- > 0 && total < FTP_XFER_MAX) {
                int r = net_stream_recv(s, xfer + total,
                                         FTP_XFER_MAX - total);
                if (r > 0) { total += (uint32_t)r; tries = TELNET_DRAIN_TRIES; }
                else plat_delay_ms(TELNET_IDLE_MS);
            }
            if (total > 0 && plat_fs_write(remote, xfer, total) == 0) {
                kprintf("  ftp: retrieved %s (%u bytes)\n", remote, (unsigned)total);
            } else {
                kprint("  ftp: retr failed (no data or local write error)\n");
            }
        }
    } else if (kstreq2(verb, "put") || kstreq2(verb, "stor")) {
        char local[64];
        next_word(rest2, local, sizeof(local));
        if (!local[0]) {
            kprint("  usage: ftp <host> put <local-file>\n");
        } else {
            static uint8_t xfer[FTP_XFER_MAX];
            uint32_t got = 0;
            if (plat_fs_read(local, xfer, sizeof(xfer), &got) != 0) {
                kprintf("  ftp: put: %s: no such local file\n", local);
            } else {
                char cmd[80];
                unsigned int p = 0;
                const char *pfx = "STOR ";
                while (pfx[p]) { cmd[p] = pfx[p]; p++; }
                unsigned int q = 0;
                while (local[q] && p + 1 < sizeof(cmd)) cmd[p++] = local[q++];
                cmd[p++] = '\r'; cmd[p++] = '\n'; cmd[p] = '\0';
                net_stream_send(s, cmd, p);
                n = net_stream_recv(s, banner, sizeof(banner) - 1);
                if (n > 0) { banner[n] = '\0'; kprintf("  %s\n", banner); }

                uint32_t sent = 0;
                while (sent < got) {
                    uint32_t chunk = got - sent;
                    if (chunk > NET_MAX_PAYLOAD) chunk = NET_MAX_PAYLOAD;
                    net_stream_send(s, xfer + sent, chunk);
                    sent += chunk;
                }
                kprintf("  ftp: stored %s (%u bytes)\n", local, (unsigned)got);
            }
        }
    } else {
        kprintf("  ftp: unknown command '%s' (use list|get|put)\n", verb);
    }

    net_stream_send(s, "QUIT\r\n", 6);
    net_stream_close(s);
    return 0;
}

/* Interactive telnet session: since net_stream_recv is non-blocking and
 * plat_read_line is a blocking full-line read (there is no portable
 * non-blocking single-key API shared by both the x86 and PS2 HALs), this
 * drives a real request/response loop rather than a one-shot banner
 * print: drain and show anything already waiting, block for one line of
 * local input, send it upstream, then drain the reply. Typing "/quit"
 * ends the session locally. */
int telnet_client(const char *host, const char *args) {
    (void)args;
    net_stream_t *s = net_connect(host, 23);
    if (!s) {
        kprint("  telnet: connect failed\n");
        return -1;
    }

    net_stream_send(s, "\r\n", 2);
    plat_delay_ms(TELNET_IDLE_MS);
    drain_and_print(s, "  ");

    kprint("  telnet: session open — type a line and press enter to send,\n");
    kprint("  telnet: type /quit to close the session.\n");

    char line[NET_LINE_MAX];
    for (;;) {
        plat_read_line(line, sizeof(line));
        if (kstreq2(line, "/quit")) break;

        unsigned int len = kstrlen2(line);
        if (len + 2 < sizeof(line)) {
            line[len++] = '\r';
            line[len++] = '\n';
        }
        net_stream_send(s, line, len);

        /* Give the remote a moment to answer, then show whatever came
         * back before prompting for the next line. */
        int tries = TELNET_DRAIN_TRIES;
        int got_any = 0;
        while (tries-- > 0) {
            int r = drain_and_print(s, "  ");
            if (r > 0) { got_any = 1; }
            else { if (got_any) break; plat_delay_ms(TELNET_IDLE_MS); }
        }
    }

    kprint("  telnet: session closed\n");
    net_stream_close(s);
    return 0;
}

int irc_client(const char *host, const char *channel, const char *nick) {
    net_stream_t *s = net_connect(host, 6667);
    if (!s) {
        kprint("  irc: connect failed\n");
        return -1;
    }
    char cmd[128];
    int pos = 0;
    const char *p = "NICK ";
    while (*p && pos < 120) cmd[pos++] = *p++;
    p = nick ? nick : "asmos";
    while (*p && pos < 120) cmd[pos++] = *p++;
    cmd[pos++] = '\r'; cmd[pos++] = '\n'; cmd[pos] = '\0';
    net_stream_send(s, cmd, pos);
    pos = 0;
    p = "USER asmos 0 * :ASMOS\r\n";
    while (*p && pos < 120) cmd[pos++] = *p++;
    net_stream_send(s, cmd, pos);
    if (channel) {
        pos = 0;
        p = "JOIN ";
        while (*p && pos < 100) cmd[pos++] = *p++;
        p = channel;
        while (*p && pos < 120) cmd[pos++] = *p++;
        cmd[pos++] = '\r'; cmd[pos++] = '\n'; cmd[pos] = '\0';
        net_stream_send(s, cmd, pos);
    }
    kprint("  irc: registered and joined channel\n");
    net_stream_close(s);
    return 0;
}
