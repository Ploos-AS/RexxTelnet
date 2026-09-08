#include <stdio.h>
#include <string.h>

#include "arexx_dispatch.h"

struct fake_state {
    int connected;
    unsigned short columns;
    unsigned short rows;
    unsigned char sent[512];
    size_t sent_len;
    char rx[256];
    int capture;
    char capture_path[128];
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
static size_t f_peek(void *ctx, char *out, size_t out_size)
{
    struct fake_state *s = (struct fake_state *)ctx;
    size_t n = strlen(s->rx);
    if (n >= out_size) n = out_size - 1u;
    memcpy(out, s->rx, n);
    out[n] = '\0';
    return n;
}
static size_t f_read(void *ctx, char *out, size_t out_size)
{
    struct fake_state *s = (struct fake_state *)ctx;
    size_t n = f_peek(ctx, out, out_size);
    if (n != 0u) memmove(s->rx, s->rx + n, strlen(s->rx + n) + 1u);
    return n;
}
static int f_waitfor(void *ctx, const char *text, unsigned long timeout)
{
    struct fake_state *s = (struct fake_state *)ctx;
    (void)timeout;
    if (strcmp(text, "cancel") == 0) return -2;
    return strstr(s->rx, text) != NULL ? 1 : 0;
}
static int f_capture_start(void *ctx, const char *path)
{
    struct fake_state *s = (struct fake_state *)ctx;
    if (s->capture) return -1;
    s->capture = 1;
    strncpy(s->capture_path, path, sizeof(s->capture_path) - 1u);
    s->capture_path[sizeof(s->capture_path) - 1u] = '\0';
    return 0;
}
static int f_capture_stop(void *ctx)
{
    struct fake_state *s = (struct fake_state *)ctx;
    if (!s->capture) return 1;
    s->capture = 0;
    return 0;
}
static int f_get_property(void *ctx, const char *name,
                          char *out, size_t out_size)
{
    struct fake_state *s = (struct fake_state *)ctx;
    const char *value;
    if (strcmp(name, "CAPTURE") == 0 || strcmp(name, "capture") == 0)
        value = s->capture ? "ON" : "OFF";
    else if (strcmp(name, "LOCAL_NAWS") == 0)
        value = "1";
    else
        return -1;
    strncpy(out, value, out_size - 1u);
    out[out_size - 1u] = '\0';
    return 0;
}

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
    const unsigned char hex_expected[] = {0x00u, 0xffu, 0x1bu, 0x41u};

    memset(&s, 0, sizeof(s));
    memset(&ops, 0, sizeof(ops));
    s.columns = 80u;
    s.rows = 25u;
    strcpy(s.rx, "banner login:");
    ops.is_connected = f_connected;
    ops.connect = f_connect;
    ops.disconnect = f_disconnect;
    ops.send = f_send;
    ops.columns = f_columns;
    ops.rows = f_rows;
    ops.set_columns = f_set_columns;
    ops.set_rows = f_set_rows;
    ops.peek = f_peek;
    ops.read = f_read;
    ops.waitfor = f_waitfor;
    ops.capture_start = f_capture_start;
    ops.capture_stop = f_capture_stop;
    ops.get_property = f_get_property;

    rt_arexx_dispatch("STATUS", &ops, &s, &r);
    ok &= expect(r.rc == 0 && strcmp(r.result, "DISCONNECTED") == 0, "status disconnected");

    rt_arexx_dispatch("CONNECT bbs.example 2323", &ops, &s, &r);
    ok &= expect(r.rc == 0 && s.connected, "connect");

    rt_arexx_dispatch("SENDLINE hello world", &ops, &s, &r);
    ok &= expect(r.rc == 0 && s.sent_len == 13u && memcmp(s.sent, "hello world\r\n", 13u) == 0, "sendline");

    rt_arexx_dispatch("SENDHEX 00 FF 1B 41", &ops, &s, &r);
    ok &= expect(r.rc == 0 && s.sent_len == sizeof(hex_expected) &&
                 memcmp(s.sent, hex_expected, sizeof(hex_expected)) == 0,
                 "sendhex spaced");

    rt_arexx_dispatch("SENDHEX 00ff1b41", &ops, &s, &r);
    ok &= expect(r.rc == 0 && s.sent_len == sizeof(hex_expected) &&
                 memcmp(s.sent, hex_expected, sizeof(hex_expected)) == 0,
                 "sendhex compact");

    rt_arexx_dispatch("WAITFOR \"login:\" 3", &ops, &s, &r);
    ok &= expect(r.rc == 0 && strcmp(r.result, "MATCH") == 0, "waitfor match");

    rt_arexx_dispatch("WAITFOR cancel 3", &ops, &s, &r);
    ok &= expect(r.rc == 5 && strcmp(r.result, "CANCELLED") == 0,
                 "waitfor cancelled");

    rt_arexx_dispatch("PEEK", &ops, &s, &r);
    ok &= expect(r.rc == 0 && strcmp(r.result, "banner login:") == 0, "peek");

    rt_arexx_dispatch("READ", &ops, &s, &r);
    ok &= expect(r.rc == 0 && strcmp(r.result, "banner login:") == 0, "read");

    rt_arexx_dispatch("CAPTURE RAM:session.log", &ops, &s, &r);
    ok &= expect(r.rc == 0 && s.capture && strcmp(s.capture_path, "RAM:session.log") == 0,
                 "capture start");

    rt_arexx_dispatch("GET capture", &ops, &s, &r);
    ok &= expect(r.rc == 0 && strcmp(r.result, "ON") == 0, "get capture");

    rt_arexx_dispatch("CAPTURE STOP", &ops, &s, &r);
    ok &= expect(r.rc == 0 && !s.capture, "capture stop");

    rt_arexx_dispatch("GET CONNECTED", &ops, &s, &r);
    ok &= expect(r.rc == 0 && strcmp(r.result, "1") == 0, "get connected");

    rt_arexx_dispatch("GET LOCAL_NAWS", &ops, &s, &r);
    ok &= expect(r.rc == 0 && strcmp(r.result, "1") == 0, "get session property");

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
