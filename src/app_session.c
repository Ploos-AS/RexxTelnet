#include "app_session.h"

struct rt_app_pump_ctx {
    struct rt_app_session *app;
    rt_terminal_emit_cb on_text;
    rt_terminal_control_cb on_control;
    void *user_ctx;
    int send_failed;
};

static void rt_app_on_data(void *opaque, unsigned char byte)
{
    struct rt_app_pump_ctx *ctx = (struct rt_app_pump_ctx *)opaque;
    rt_rx_buffer_append(&ctx->app->rx_buffer, &byte, 1u);
    if (ctx->app->on_app_data != NULL)
        ctx->app->on_app_data(ctx->app->app_data_ctx, &byte, 1u);
    rt_terminal_feed(&ctx->app->terminal, &byte, 1u,
                     ctx->on_text, ctx->on_control, ctx->user_ctx);
}

static void rt_app_send_naws(struct rt_app_pump_ctx *ctx)
{
    unsigned char naws[16];
    size_t len;
    long written;

    len = rt_terminal_build_naws(ctx->app->terminal.columns,
                                 ctx->app->terminal.rows,
                                 naws, sizeof(naws));
    if (len == 0u) {
        ctx->send_failed = 1;
        return;
    }

    written = rt_transport_send(&ctx->app->transport, naws, len);
    if (written != (long)len) ctx->send_failed = 1;
}

static void rt_app_on_negotiation(void *opaque,
                                  unsigned char verb,
                                  unsigned char option)
{
    struct rt_app_pump_ctx *ctx = (struct rt_app_pump_ctx *)opaque;
    struct rt_reply reply;
    unsigned char had_naws;
    long written;

    had_naws = ctx->app->session.local_naws;
    reply = rt_session_negotiate(&ctx->app->session, verb, option);

    if (reply.len != 0u) {
        written = rt_transport_send(&ctx->app->transport,
                                    reply.bytes, reply.len);
        if (written != (long)reply.len) {
            ctx->send_failed = 1;
            return;
        }
    }

    if (!had_naws && ctx->app->session.local_naws)
        rt_app_send_naws(ctx);
}

void rt_app_session_init(struct rt_app_session *app,
                         unsigned short columns,
                         unsigned short rows)
{
    if (app == NULL) return;
    rt_transport_init(&app->transport);
    rt_session_init(&app->session);
    rt_terminal_init(&app->terminal, columns, rows);
    rt_rx_buffer_init(&app->rx_buffer);
    app->on_app_data = NULL;
    app->app_data_ctx = NULL;
}

void rt_app_session_set_data_observer(struct rt_app_session *app,
                                      rt_app_data_cb callback,
                                      void *ctx)
{
    if (app == NULL) return;
    app->on_app_data = callback;
    app->app_data_ctx = ctx;
}

int rt_app_session_connect(struct rt_app_session *app,
                           const char *host,
                           unsigned short port)
{
    if (app == NULL) return -1;
    rt_session_init(&app->session);
    rt_rx_buffer_init(&app->rx_buffer);
    return rt_transport_connect(&app->transport, host, port);
}

long rt_app_session_send_input(struct rt_app_session *app,
                               const unsigned char *data,
                               size_t len)
{
    unsigned char encoded[512];
    size_t encoded_len;

    if (app == NULL || data == NULL) return -1;
    if (len == 0u) return 0;
    if (len > sizeof(encoded) / 2u) return -1;

    encoded_len = rt_telnet_encode_data(data, len,
                                        encoded, sizeof(encoded));
    if (encoded_len == 0u) return -1;
    return rt_transport_send(&app->transport, encoded, encoded_len);
}

long rt_app_session_pump(struct rt_app_session *app,
                         rt_terminal_emit_cb on_text,
                         rt_terminal_control_cb on_control,
                         void *ctx)
{
    unsigned char input[256];
    long received;
    struct rt_app_pump_ctx pump;

    if (app == NULL) return -1;

    received = rt_transport_recv(&app->transport, input, sizeof(input));
    if (received <= 0) return received;

    pump.app = app;
    pump.on_text = on_text;
    pump.on_control = on_control;
    pump.user_ctx = ctx;
    pump.send_failed = 0;

    rt_telnet_feed(&app->session.parser, input, (size_t)received,
                   rt_app_on_data, rt_app_on_negotiation, &pump);

    if (pump.send_failed) return -1;
    return received;
}

void rt_app_session_disconnect(struct rt_app_session *app)
{
    if (app == NULL) return;
    rt_transport_disconnect(&app->transport);
    rt_session_init(&app->session);
}
