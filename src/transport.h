#ifndef REXXTELNET_TRANSPORT_H
#define REXXTELNET_TRANSPORT_H

#include <stddef.h>

struct rt_transport {
    int socket_fd;
    int connected;
};

void rt_transport_init(struct rt_transport *transport);
int rt_transport_connect(struct rt_transport *transport,
                         const char *host,
                         unsigned short port);
long rt_transport_send(struct rt_transport *transport,
                       const unsigned char *data,
                       size_t len);
long rt_transport_recv(struct rt_transport *transport,
                       unsigned char *data,
                       size_t len);
int rt_transport_readable(struct rt_transport *transport);
void rt_transport_disconnect(struct rt_transport *transport);

#endif
