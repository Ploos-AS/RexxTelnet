#include "transport.h"

long rt_transport_send_all(struct rt_transport *transport,
                           const unsigned char *data,
                           size_t len)
{
    size_t sent = 0u;

    if (transport == NULL || data == NULL) return -1;
    if (len == 0u) return 0;

    while (sent < len) {
        long rc = rt_transport_send(transport, data + sent, len - sent);
        if (rc <= 0) return -1;
        sent += (size_t)rc;
    }

    return (long)sent;
}
