#include <stdio.h>
#include <string.h>

#include "rx_buffer.h"

static int expect(int cond, const char *name)
{
    if (!cond) { fprintf(stderr, "FAIL: %s\n", name); return 0; }
    return 1;
}

int main(void)
{
    struct rt_rx_buffer b;
    char out[32];
    unsigned char large[RT_RX_BUFFER_SIZE + 10u];
    size_t i;
    int ok = 1;

    rt_rx_buffer_init(&b);
    rt_rx_buffer_append(&b, (const unsigned char *)"hello login: rest", 17u);

    ok &= expect(rt_rx_buffer_contains(&b, "login:"), "contains");
    ok &= expect(rt_rx_buffer_peek(&b, out, sizeof(out)) == 17u &&
                 strcmp(out, "hello login: rest") == 0, "peek");
    ok &= expect(rt_rx_buffer_consume_through(&b, "login:"), "consume through");
    ok &= expect(rt_rx_buffer_peek(&b, out, sizeof(out)) == 5u &&
                 strcmp(out, " rest") == 0, "remaining");
    ok &= expect(rt_rx_buffer_read(&b, out, sizeof(out)) == 5u &&
                 strcmp(out, " rest") == 0, "read");
    ok &= expect(rt_rx_buffer_peek(&b, out, sizeof(out)) == 0u &&
                 strcmp(out, "") == 0, "empty after read");

    rt_rx_buffer_init(&b);
    for (i = 0u; i < sizeof(large); ++i) large[i] = (unsigned char)(i & 0xffu);
    rt_rx_buffer_append(&b, large, sizeof(large));
    ok &= expect(b.len == RT_RX_BUFFER_SIZE, "overflow retains bounded size");
    ok &= expect(b.dropped == 10u, "overflow counts dropped bytes");
    ok &= expect(b.data[0] == large[10], "overflow retains newest bytes");

    rt_rx_buffer_init(&b);
    rt_rx_buffer_append(&b, large, RT_RX_BUFFER_SIZE - 4u);
    rt_rx_buffer_append(&b, large, 8u);
    ok &= expect(b.len == RT_RX_BUFFER_SIZE, "incremental overflow size");
    ok &= expect(b.dropped == 4u, "incremental overflow count");

    if (!ok) return 1;
    puts("PASS: rx buffer");
    return 0;
}
