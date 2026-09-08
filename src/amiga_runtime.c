#include "amiga_runtime.h"

#ifdef __AMIGA__

#include <proto/dos.h>
#include <stdio.h>

#include "app_session.h"
#include "arexx_amiga.h"
#include "terminal_amiga.h"

struct rt_amiga_display_ctx {
    int failed;
};

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

static int ar_is_connected(void *opaque)
{
    return ((struct rt_app_session *)opaque)->transport.connected;
}

static int ar_connect(void *opaque, const char *host, unsigned short port)
{
    return rt_app_session_connect((struct rt_app_session *)opaque, host, port);
}

static void ar_disconnect(void *opaque)
{
    rt_app_session_disconnect((struct rt_app_session *)opaque);
}

static long ar_send(void *opaque, const unsigned char *data, size_t len)
{
    return rt_app_session_send_input((struct rt_app_session *)opaque, data, len);
}

static unsigned short ar_columns(void *opaque)
{
    return ((struct rt_app_session *)opaque)->terminal.columns;
}

static unsigned short ar_rows(void *opaque)
{
    return ((struct rt_app_session *)opaque)->terminal.rows;
}

static int ar_set_columns(void *opaque, unsigned short value)
{
    ((struct rt_app_session *)opaque)->terminal.columns = value;
    return 0;
}

static int ar_set_rows(void *opaque, unsigned short value)
{
    ((struct rt_app_session *)opaque)->terminal.rows = value;
    return 0;
}

int rt_amiga_run(const char *host, unsigned short port)
{
    struct rt_app_session app;
    struct rt_amiga_display_ctx display;
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

    arexx_ops.is_connected = ar_is_connected;
    arexx_ops.connect = ar_connect;
    arexx_ops.disconnect = ar_disconnect;
    arexx_ops.send = ar_send;
    arexx_ops.columns = ar_columns;
    arexx_ops.rows = ar_rows;
    arexx_ops.set_columns = ar_set_columns;
    arexx_ops.set_rows = ar_set_rows;

    display.failed = 0;

    while (running) {
        int did_work = 0;

        if (rt_arexx_port_pending(arexx)) {
            int quit_requested = 0;
            if (rt_arexx_port_process(arexx, &arexx_ops, &app,
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
            if (rc <= 0 || display.failed) {
                rt_app_session_disconnect(&app);
            }
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
