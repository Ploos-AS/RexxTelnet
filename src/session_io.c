#include "session_io.h"

struct rt_pump_ctx {
    struct rt_session_io *io;
    rt_session_data_cb on_data;
    void *user_ctx;
    unsigned char data[256];
    size_t data_len;
    int send_failed;
};

static void flush_data(struct rt_pump_ctx *ctx)
{
    if (ctx->data_len != 0u && ctx->on_data != NULL) {
        ctx->on_data(ctx->user_ctx, ctx->data, ctx->data_len);
    }
    ctx->data_len = 0u;
}

static void on_telnet_data(void *opaque, unsigned char byte)
{
    struct rt_pump_ctx *ctx = (struct rt_pump_ctx *)opaque;

    if (ctx->data_len == sizeof(ctx->data)) flush_data(ctx);
    ctx->data[ctx->data_len++] = byte;
}

static void on_telnet_negotiation(void *opaque,
                                  unsigned char verb,
                                  unsigned char option)
{
    struct rt_pump_ctx *ctx = (struct rt_pump_ctx *)opaque;
    struct rt_reply reply;

    flush_data(ctx);
    reply = rt_session_negotiate(&ctx->io->session, verb, option);
    if (reply.len != 0u) {
        long written = rt_transport_send(&ctx->io->transport,
                                         reply.bytes,
                                         reply.len);
        if (written != (long)reply.len) ctx->send_failed = 1;
    }
}

void rt_session_io_init(struct rt_session_io *io)
{
    if (io != NULL) {
        rt_session_init(&io->session);
        rt_transport_init(&io->transport);
    }
}

int rt_session_io_connect(struct rt_session_io *io,
                          const char *host,
                          unsigned short port)
{
    if (io == NULL) return -1;
    rt_session_init(&io->session);
    return rt_transport_connect(&io->transport, host, port);
}

void rt_session_io_disconnect(struct rt_session_io *io)
{
    if (io == NULL) return;
    rt_transport_disconnect(&io->transport);
    rt_session_init(&io->session);
}

long rt_session_io_send(struct rt_session_io *io,
                        const unsigned char *data,
                        size_t len)
{
    unsigned char encoded[512];
    size_t encoded_len;

    if (io == NULL || data == NULL) return -1;
    if (len == 0u) return 0;
    if (len > sizeof(encoded) / 2u) return -1;

    encoded_len = rt_telnet_encode_data(data, len, encoded, sizeof(encoded));
    if (encoded_len == 0u) return -1;
    return rt_transport_send(&io->transport, encoded, encoded_len);
}

long rt_session_io_pump(struct rt_session_io *io,
                        rt_session_data_cb on_data,
                        void *ctx)
{
    unsigned char input[256];
    long received;
    struct rt_pump_ctx pump;

    if (io == NULL) return -1;

    received = rt_transport_recv(&io->transport, input, sizeof(input));
    if (received <= 0) return received;

    pump.io = io;
    pump.on_data = on_data;
    pump.user_ctx = ctx;
    pump.data_len = 0u;
    pump.send_failed = 0;

    rt_telnet_feed(&io->session.parser, input, (size_t)received,
                   on_telnet_data, on_telnet_negotiation, &pump);
    flush_data(&pump);

    if (pump.send_failed) return -1;
    return received;
}
