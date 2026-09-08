#include "amiga_runtime.h"

#ifdef __AMIGA__

#include <proto/dos.h>
#include <stdio.h>

#include "app_session.h"
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

int rt_amiga_run(const char *host, unsigned short port)
{
    struct rt_app_session app;
    struct rt_amiga_display_ctx display;
    unsigned char input[64];
    int running = 1;

    if (host == NULL || port == 0u) return 20;
    if (rt_terminal_amiga_open() != 0) return 20;

    rt_app_session_init(&app, 80u, 25u);
    if (rt_app_session_connect(&app, host, port) != 0) {
        rt_terminal_amiga_close();
        return 10;
    }

    display.failed = 0;

    while (running) {
        int did_work = 0;

        if (rt_transport_readable(&app.transport)) {
            long rc = rt_app_session_pump(&app,
                                          rt_amiga_emit_text,
                                          rt_amiga_emit_control,
                                          &display);
            did_work = 1;
            if (rc <= 0 || display.failed) running = 0;
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
                if (running && rt_app_session_send_input(&app, input,
                                                         (size_t)count) < 0)
                    running = 0;
            }
        }

        if (!did_work) Delay(1L);
    }

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
