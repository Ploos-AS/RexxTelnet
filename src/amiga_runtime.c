#include "amiga_runtime.h"

#ifdef __AMIGA__

#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include "app_session.h"
#include "arexx_amiga.h"
#include "terminal_amiga.h"

struct rt_amiga_display_ctx {
    int failed;
};

struct rt_amiga_arexx_ctx {
    struct rt_app_session *app;
    struct rt_amiga_display_ctx *display;
    BPTR capture_file;
    char capture_path[256];
    int capture_failed;
};

static int rt_equals_ci(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;
    while (*a != '\0' && *b != '\0') {
        ca = (unsigned char)*a;
        cb = (unsigned char)*b;
        if (ca >= 'a' && ca <= 'z') ca = (unsigned char)(ca - 'a' + 'A');
        if (cb >= 'a' && cb <= 'z') cb = (unsigned char)(cb - 'a' + 'A');
        if (ca != cb) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static void rt_amiga_emit_text(void *opaque, unsigned char byte)
{
    struct rt_amiga_display_ctx *ctx =
        (struct rt_amiga_display_ctx *)opaque;
    if (rt_terminal_amiga_write(&byte, 1u) != 1L) ctx->failed = 1;
}

static void rt_amiga_emit_control(void *opaque,
                                  unsigned char command,
                                  const unsigned int *params,
                                  unsigned char param_count)
{
    struct rt_amiga_display_ctx *ctx =
        (struct rt_amiga_display_ctx *)opaque;
    char sequence[48];
    int pos = 0;
    unsigned char i;

    sequence[pos++] = 27;
    sequence[pos++] = '[';
    for (i = 0u; i < param_count && pos < (int)sizeof(sequence) - 8; ++i) {
        if (i != 0u) sequence[pos++] = ';';
        pos += sprintf(sequence + pos, "%u", params[i]);
    }
    sequence[pos++] = (char)command;

    if (rt_terminal_amiga_write((const unsigned char *)sequence,
                                (unsigned long)pos) != (long)pos)
        ctx->failed = 1;
}

static void rt_amiga_capture_data(void *opaque,
                                  const unsigned char *data,
                                  size_t len)
{
    struct rt_amiga_arexx_ctx *ctx = (struct rt_amiga_arexx_ctx *)opaque;
    if (ctx == NULL || ctx->capture_file == 0 || data == NULL || len == 0u)
        return;
    if (Write(ctx->capture_file, (APTR)data, (LONG)len) != (LONG)len) {
        Close(ctx->capture_file);
        ctx->capture_file = 0;
        ctx->capture_failed = 1;
    }
}

static struct rt_app_session *ar_app(void *opaque)
{
    return ((struct rt_amiga_arexx_ctx *)opaque)->app;
}

static int ar_is_connected(void *opaque)
{
    return ar_app(opaque)->transport.connected;
}

static int ar_connect(void *opaque, const char *host, unsigned short port)
{
    return rt_app_session_connect(ar_app(opaque), host, port);
}

static void ar_disconnect(void *opaque)
{
    rt_app_session_disconnect(ar_app(opaque));
}

static long ar_send(void *opaque, const unsigned char *data, size_t len)
{
    return rt_app_session_send_input(ar_app(opaque), data, len);
}

static unsigned short ar_columns(void *opaque)
{
    return ar_app(opaque)->terminal.columns;
}

static unsigned short ar_rows(void *opaque)
{
    return ar_app(opaque)->terminal.rows;
}

static int ar_set_columns(void *opaque, unsigned short value)
{
    ar_app(opaque)->terminal.columns = value;
    return 0;
}

static int ar_set_rows(void *opaque, unsigned short value)
{
    ar_app(opaque)->terminal.rows = value;
    return 0;
}

static size_t ar_peek(void *opaque, char *output, size_t output_size)
{
    return rt_rx_buffer_peek(&ar_app(opaque)->rx_buffer, output, output_size);
}

static size_t ar_read(void *opaque, char *output, size_t output_size)
{
    return rt_rx_buffer_read(&ar_app(opaque)->rx_buffer, output, output_size);
}

static int ar_waitfor(void *opaque, const char *text,
                      unsigned long timeout_seconds)
{
    struct rt_amiga_arexx_ctx *ctx = (struct rt_amiga_arexx_ctx *)opaque;
    struct rt_app_session *app = ctx->app;
    unsigned long ticks = timeout_seconds * 50u;
    unsigned long elapsed = 0u;

    if (rt_rx_buffer_consume_through(&app->rx_buffer, text)) return 1;

    while (elapsed < ticks) {
        if (!app->transport.connected) return -1;

        if (rt_transport_readable(&app->transport)) {
            long rc = rt_app_session_pump(app,
                                          rt_amiga_emit_text,
                                          rt_amiga_emit_control,
                                          ctx->display);
            if (rc <= 0 || ctx->display->failed) {
                rt_app_session_disconnect(app);
                return -1;
            }
            if (rt_rx_buffer_consume_through(&app->rx_buffer, text))
                return 1;
        }

        Delay(1L);
        ++elapsed;
    }

    return 0;
}

static int ar_capture_start(void *opaque, const char *path)
{
    struct rt_amiga_arexx_ctx *ctx = (struct rt_amiga_arexx_ctx *)opaque;
    BPTR file;
    if (ctx == NULL || path == NULL || *path == '\0') return -1;
    if (ctx->capture_file != 0) return -1;
    file = Open((STRPTR)path, MODE_NEWFILE);
    if (file == 0) return -1;
    ctx->capture_file = file;
    strncpy(ctx->capture_path, path, sizeof(ctx->capture_path) - 1u);
    ctx->capture_path[sizeof(ctx->capture_path) - 1u] = '\0';
    ctx->capture_failed = 0;
    return 0;
}

static int ar_capture_stop(void *opaque)
{
    struct rt_amiga_arexx_ctx *ctx = (struct rt_amiga_arexx_ctx *)opaque;
    if (ctx == NULL || ctx->capture_file == 0) return 1;
    Close(ctx->capture_file);
    ctx->capture_file = 0;
    return 0;
}

static int ar_get_property(void *opaque, const char *name,
                           char *output, size_t output_size)
{
    struct rt_amiga_arexx_ctx *ctx = (struct rt_amiga_arexx_ctx *)opaque;
    struct rt_app_session *app = ctx->app;
    const char *text = NULL;

    if (output == NULL || output_size == 0u || name == NULL) return -1;
    if (rt_equals_ci(name, "LOCAL_BINARY"))
        text = app->session.local_binary ? "1" : "0";
    else if (rt_equals_ci(name, "REMOTE_BINARY"))
        text = app->session.remote_binary ? "1" : "0";
    else if (rt_equals_ci(name, "REMOTE_ECHO"))
        text = app->session.remote_echo ? "1" : "0";
    else if (rt_equals_ci(name, "LOCAL_SGA"))
        text = app->session.local_sga ? "1" : "0";
    else if (rt_equals_ci(name, "REMOTE_SGA"))
        text = app->session.remote_sga ? "1" : "0";
    else if (rt_equals_ci(name, "LOCAL_NAWS"))
        text = app->session.local_naws ? "1" : "0";
    else if (rt_equals_ci(name, "CAPTURE"))
        text = ctx->capture_file != 0 ? "ON" : "OFF";
    else if (rt_equals_ci(name, "CAPTUREFILE"))
        text = ctx->capture_path;
    else if (rt_equals_ci(name, "CAPTUREERROR"))
        text = ctx->capture_failed ? "1" : "0";
    else if (rt_equals_ci(name, "RXBYTES")) {
        sprintf(output, "%lu", (unsigned long)app->rx_buffer.len);
        return 0;
    } else {
        return -1;
    }

    strncpy(output, text, output_size - 1u);
    output[output_size - 1u] = '\0';
    return 0;
}

int rt_amiga_run(const char *host, unsigned short port)
{
    struct rt_app_session app;
    struct rt_amiga_display_ctx display;
    struct rt_amiga_arexx_ctx arexx_ctx;
    struct rt_arexx_port *arexx;
    struct rt_arexx_ops arexx_ops;
    unsigned char input[64];
    int running = 1;

    if (host == NULL || port == 0u) return 20;
    if (rt_terminal_amiga_open() != 0) return 20;

    rt_app_session_init(&app, 80u, 25u);
    if (rt_app_session_connect(&app, host, port) != 0) {
        rt_terminal_amiga_close();
        return 10;
    }

    arexx = rt_arexx_port_open();
    if (arexx == NULL) {
        rt_app_session_disconnect(&app);
        rt_terminal_amiga_close();
        return 20;
    }

    display.failed = 0;
    arexx_ctx.app = &app;
    arexx_ctx.display = &display;
    arexx_ctx.capture_file = 0;
    arexx_ctx.capture_path[0] = '\0';
    arexx_ctx.capture_failed = 0;
    rt_app_session_set_data_observer(&app, rt_amiga_capture_data, &arexx_ctx);

    arexx_ops.is_connected = ar_is_connected;
    arexx_ops.connect = ar_connect;
    arexx_ops.disconnect = ar_disconnect;
    arexx_ops.send = ar_send;
    arexx_ops.columns = ar_columns;
    arexx_ops.rows = ar_rows;
    arexx_ops.set_columns = ar_set_columns;
    arexx_ops.set_rows = ar_set_rows;
    arexx_ops.peek = ar_peek;
    arexx_ops.read = ar_read;
    arexx_ops.waitfor = ar_waitfor;
    arexx_ops.capture_start = ar_capture_start;
    arexx_ops.capture_stop = ar_capture_stop;
    arexx_ops.get_property = ar_get_property;

    while (running) {
        int did_work = 0;

        if (rt_arexx_port_pending(arexx)) {
            int quit_requested = 0;
            if (rt_arexx_port_process(arexx, &arexx_ops, &arexx_ctx,
                                      &quit_requested) < 0)
                running = 0;
            if (quit_requested) running = 0;
            did_work = 1;
        }

        if (running && app.transport.connected &&
            rt_transport_readable(&app.transport)) {
            long rc = rt_app_session_pump(&app,
                                          rt_amiga_emit_text,
                                          rt_amiga_emit_control,
                                          &display);
            did_work = 1;
            if (rc <= 0 || display.failed)
                rt_app_session_disconnect(&app);
        }

        if (running && rt_terminal_amiga_has_input()) {
            long count = rt_terminal_amiga_read(input, sizeof(input));
            long i;
            did_work = 1;

            if (count <= 0) {
                running = 0;
            } else {
                for (i = 0; i < count; ++i) {
                    if (input[i] == 29u) {
                        running = 0;
                        break;
                    }
                }
                if (running && app.transport.connected &&
                    rt_app_session_send_input(&app, input,
                                              (size_t)count) < 0)
                    rt_app_session_disconnect(&app);
            }
        }

        if (!did_work) Delay(1L);
    }

    if (arexx_ctx.capture_file != 0) Close(arexx_ctx.capture_file);
    rt_arexx_port_close(arexx);
    rt_app_session_disconnect(&app);
    rt_terminal_amiga_close();
    return display.failed ? 20 : 0;
}

#else

int rt_amiga_run(const char *host, unsigned short port)
{
    (void)host;
    (void)port;
    return 20;
}

#endif
