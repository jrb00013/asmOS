#ifndef NET_CLIENTS_H
#define NET_CLIENTS_H

/* Minimal network client implementations for shell commands. */
int ftp_client(const char *host, const char *args);
int telnet_client(const char *host, const char *args);
int irc_client(const char *host, const char *channel, const char *nick);

#endif /* NET_CLIENTS_H */
