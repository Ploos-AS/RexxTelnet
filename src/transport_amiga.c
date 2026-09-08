#include "transport.h"

#ifdef __AMIGA__

#include <proto/exec.h>
#include <proto/socket.h>
#include <proto/bsdsocket.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

struct Library *SocketBase = NULL;

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
    struct hostent *entry;
    struct sockaddr_in address;
    int fd;

    if (transport == NULL || host == NULL || port == 0u) return -1;
    if (transport->connected) return -1;

    if (SocketBase == NULL) {
        SocketBase = OpenLibrary("bsdsocket.library", 4);
        if (SocketBase == NULL) return -1;
    }

    entry = gethostbyname(host);
    if (entry == NULL || entry->h_addr_list == NULL ||
        entry->h_addr_list[0] == NULL) return -1;

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    memcpy(&address.sin_addr, entry->h_addr_list[0], sizeof(address.sin_addr));

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        CloseSocket(fd);
        return -1;
    }

    transport->socket_fd = fd;
    transport->connected = 1;
    return 0;
}

long rt_transport_send(struct rt_transport *transport,
                       const unsigned char *data,
                       size_t len)
{
    if (transport == NULL || !transport->connected || data == NULL) return -1;
    return (long)send(transport->socket_fd, (const char *)data, len, 0);
}

long rt_transport_recv(struct rt_transport *transport,
                       unsigned char *data,
                       size_t len)
{
    if (transport == NULL || !transport->connected || data == NULL) return -1;
    return (long)recv(transport->socket_fd, (char *)data, len, 0);
}

int rt_transport_readable(struct rt_transport *transport)
{
    fd_set readfds;
    struct timeval timeout;
    int rc;

    if (transport == NULL || !transport->connected ||
        transport->socket_fd < 0) return 0;

    FD_ZERO(&readfds);
    FD_SET(transport->socket_fd, &readfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    rc = WaitSelect(transport->socket_fd + 1, &readfds,
                    NULL, NULL, &timeout, NULL);
    if (rc <= 0) return 0;
    return FD_ISSET(transport->socket_fd, &readfds) ? 1 : 0;
}

void rt_transport_disconnect(struct rt_transport *transport)
{
    if (transport == NULL) return;
    if (transport->socket_fd >= 0) CloseSocket(transport->socket_fd);
    transport->socket_fd = -1;
    transport->connected = 0;
}

#endif
