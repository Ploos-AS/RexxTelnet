#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transport.h"

static unsigned char captured[32];
static size_t captured_len;
static size_t chunk_limit = 2u;

long rt_transport_send(struct rt_transport *transport,
                       const unsigned char *data,
                       size_t len)
{
    size_t n;
    if (transport == NULL || !transport->connected || data == NULL) return -1;
    n = len < chunk_limit ? len : chunk_limit;
    if (captured_len + n > sizeof(captured)) return -1;
    memcpy(captured + captured_len, data, n);
    captured_len += n;
    return (long)n;
}

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(void)
{
    struct rt_transport transport;
    const unsigned char payload[] = {'A','B','C','D','E'};

    transport.socket_fd = 1;
    transport.connected = 1;
    captured_len = 0u;

    require(rt_transport_send_all(&transport, payload, sizeof(payload)) == 5L,
            "send-all length");
    require(captured_len == sizeof(payload), "captured length");
    require(memcmp(captured, payload, sizeof(payload)) == 0, "captured payload");

    transport.connected = 0;
    require(rt_transport_send_all(&transport, payload, sizeof(payload)) < 0,
            "send-all failure");

    puts("PASS: transport common");
    return 0;
}
