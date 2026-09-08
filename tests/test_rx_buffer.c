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

    if (!ok) return 1;
    puts("PASS: rx buffer");
    return 0;
}
