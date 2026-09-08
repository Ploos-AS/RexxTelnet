#include <stdio.h>
#include <string.h>

#include "arexx_dispatch.h"

struct fake_state {
    int connected;
    unsigned short columns;
    unsigned short rows;
    char sent[512];
    size_t sent_len;
};

static int f_connected(void *ctx) { return ((struct fake_state *)ctx)->connected; }
static int f_connect(void *ctx, const char *host, unsigned short port)
{
    struct fake_state *s = (struct fake_state *)ctx;
    if (strcmp(host, "bbs.example") != 0 || port != 2323u) return -1;
    s->connected = 1;
    return 0;
}
static void f_disconnect(void *ctx) { ((struct fake_state *)ctx)->connected = 0; }
static long f_send(void *ctx, const unsigned char *data, size_t len)
{
    struct fake_state *s = (struct fake_state *)ctx;
    if (len > sizeof(s->sent)) return -1;
    memcpy(s->sent, data, len);
    s->sent_len = len;
    return (long)len;
}
static unsigned short f_columns(void *ctx) { return ((struct fake_state *)ctx)->columns; }
static unsigned short f_rows(void *ctx) { return ((struct fake_state *)ctx)->rows; }
static int f_set_columns(void *ctx, unsigned short v) { ((struct fake_state *)ctx)->columns = v; return 0; }
static int f_set_rows(void *ctx, unsigned short v) { ((struct fake_state *)ctx)->rows = v; return 0; }

static int expect(int cond, const char *name)
{
    if (!cond) { fprintf(stderr, "FAIL: %s\n", name); return 0; }
    return 1;
}

int main(void)
{
    struct fake_state s;
    struct rt_arexx_ops ops;
    struct rt_arexx_result r;
    int ok = 1;

    memset(&s, 0, sizeof(s));
    s.columns = 80u;
    s.rows = 25u;
    ops.is_connected = f_connected;
    ops.connect = f_connect;
    ops.disconnect = f_disconnect;
    ops.send = f_send;
    ops.columns = f_columns;
    ops.rows = f_rows;
    ops.set_columns = f_set_columns;
    ops.set_rows = f_set_rows;

    rt_arexx_dispatch("STATUS", &ops, &s, &r);
    ok &= expect(r.rc == 0 && strcmp(r.result, "DISCONNECTED") == 0, "status disconnected");

    rt_arexx_dispatch("CONNECT bbs.example 2323", &ops, &s, &r);
    ok &= expect(r.rc == 0 && s.connected, "connect");

    rt_arexx_dispatch("SENDLINE hello world", &ops, &s, &r);
    ok &= expect(r.rc == 0 && s.sent_len == 13u && memcmp(s.sent, "hello world\r\n", 13u) == 0, "sendline");

    rt_arexx_dispatch("GET columns", &ops, &s, &r);
    ok &= expect(r.rc == 0 && strcmp(r.result, "80") == 0, "get columns");

    rt_arexx_dispatch("SET rows 40", &ops, &s, &r);
    ok &= expect(r.rc == 0 && s.rows == 40u, "set rows");

    rt_arexx_dispatch("DISCONNECT", &ops, &s, &r);
    ok &= expect(r.rc == 0 && !s.connected, "disconnect");

    rt_arexx_dispatch("QUIT", &ops, &s, &r);
    ok &= expect(r.rc == 0 && r.quit, "quit");

    if (!ok) return 1;
    puts("PASS: arexx dispatch");
    return 0;
}
