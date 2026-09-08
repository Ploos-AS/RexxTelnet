#include "transport.h"

#ifndef __AMIGA__

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void rt_transport_init(struct rt_transport *transport)
{
    if (transport != NULL) {
        transport->socket_fd = -1;
        transport->connected = 0;
    }
}

int rt_transport_connect(struct rt_transport *transport,
                         const char *host,
                         unsigned short port)
{
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *item;
    char service[6];
    int rc;

    if (transport == NULL || host == NULL || port == 0u) return -1;
    if (transport->connected) return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    (void)snprintf(service, sizeof(service), "%u", (unsigned int)port);

    rc = getaddrinfo(host, service, &hints, &result);
    if (rc != 0) return -1;

    for (item = result; item != NULL; item = item->ai_next) {
        int fd = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, item->ai_addr, item->ai_addrlen) == 0) {
            transport->socket_fd = fd;
            transport->connected = 1;
            freeaddrinfo(result);
            return 0;
        }
        close(fd);
    }

    freeaddrinfo(result);
    return -1;
}

long rt_transport_send(struct rt_transport *transport,
                       const unsigned char *data,
                       size_t len)
{
    if (transport == NULL || !transport->connected || data == NULL) return -1;
    return (long)send(transport->socket_fd, data, len, 0);
}

long rt_transport_recv(struct rt_transport *transport,
                       unsigned char *data,
                       size_t len)
{
    if (transport == NULL || !transport->connected || data == NULL) return -1;
    return (long)recv(transport->socket_fd, data, len, 0);
}

void rt_transport_disconnect(struct rt_transport *transport)
{
    if (transport == NULL) return;
    if (transport->socket_fd >= 0) close(transport->socket_fd);
    transport->socket_fd = -1;
    transport->connected = 0;
}

#endif
