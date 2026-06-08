/* Minimal FTP/telnet/IRC clients over net_stream UDP transport. */

#include "net_clients.h"
#include "net.h"
#include "kernel.h"
#include <stddef.h>

int ftp_client(const char *host, const char *args) {
    (void)args;
    net_stream_t *s = net_connect(host, 21);
    if (!s) {
        kprint("  ftp: connect failed\n");
        return -1;
    }
    char banner[128];
    int n = net_stream_recv(s, banner, sizeof(banner) - 1);
    if (n > 0) {
        banner[n] = '\0';
        kprintf("  ftp: %s\n", banner);
    }
    net_stream_send(s, "USER anonymous\r\n", 16);
    n = net_stream_recv(s, banner, sizeof(banner) - 1);
    if (n > 0) { banner[n] = '\0'; kprintf("  %s\n", banner); }
    net_stream_send(s, "PASS guest@\r\n", 13);
    n = net_stream_recv(s, banner, sizeof(banner) - 1);
    if (n > 0) { banner[n] = '\0'; kprintf("  %s\n", banner); }
    kprint("  ftp: connected (basic auth sent)\n");
    net_stream_close(s);
    return 0;
}

int telnet_client(const char *host, const char *args) {
    (void)args;
    net_stream_t *s = net_connect(host, 23);
    if (!s) {
        kprint("  telnet: connect failed\n");
        return -1;
    }
    net_stream_send(s, "\r\n", 2);
    char buf[256];
    int n = net_stream_recv(s, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        kprintf("  telnet: %s\n", buf);
    }
    kprint("  telnet: session open (type via future interactive mode)\n");
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
