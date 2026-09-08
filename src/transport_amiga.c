#include "transport.h"

#ifdef __AMIGA__

#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <libraries/bsdsocket.h>
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
    LONG fd;

    if (transport == NULL || host == NULL || port == 0u) return -1;
    if (transport->connected) return -1;
    if (transport->socket_fd >= 0) rt_transport_disconnect(transport);

    if (SocketBase == NULL) {
        SocketBase = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
        if (SocketBase == NULL) return -1;
    }

    entry = gethostbyname((STRPTR)host);
    if (entry == NULL || entry->h_addr_list == NULL ||
        entry->h_addr_list[0] == NULL) {
        rt_transport_disconnect(transport);
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    memcpy(&address.sin_addr, entry->h_addr_list[0], sizeof(address.sin_addr));

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        rt_transport_disconnect(transport);
        return -1;
    }
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        CloseSocket(fd);
        rt_transport_disconnect(transport);
        return -1;
    }

    transport->socket_fd = (int)fd;
    transport->connected = 1;
    return 0;
}

long rt_transport_send(struct rt_transport *transport,
                       const unsigned char *data,
                       size_t len)
{
    if (transport == NULL || !transport->connected || data == NULL) return -1;
    return (long)send((LONG)transport->socket_fd, (char *)data, len, 0);
}

long rt_transport_recv(struct rt_transport *transport,
                       unsigned char *data,
                       size_t len)
{
    if (transport == NULL || !transport->connected || data == NULL) return -1;
    return (long)recv((LONG)transport->socket_fd, (char *)data, len, 0);
}

int rt_transport_readable(struct rt_transport *transport)
{
    fd_set readfds;
    struct timeval timeout;
    LONG rc;

    if (transport == NULL || !transport->connected ||
        transport->socket_fd < 0) return 0;

    FD_ZERO(&readfds);
    FD_SET(transport->socket_fd, &readfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    rc = WaitSelect((LONG)transport->socket_fd + 1, &readfds,
                    NULL, NULL, &timeout, NULL);
    if (rc <= 0) return 0;
    return FD_ISSET(transport->socket_fd, &readfds) ? 1 : 0;
}

void rt_transport_disconnect(struct rt_transport *transport)
{
    if (transport != NULL) {
        if (transport->socket_fd >= 0) CloseSocket((LONG)transport->socket_fd);
        transport->socket_fd = -1;
        transport->connected = 0;
    }
    if (SocketBase != NULL) {
        CloseLibrary(SocketBase);
        SocketBase = NULL;
    }
}

#endif
