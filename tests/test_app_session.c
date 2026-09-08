#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_session.h"

static unsigned char fake_rx[256];
static size_t fake_rx_len;
static unsigned char fake_tx[512];
static size_t fake_tx_len;
static int disconnect_count;

void rt_transport_init(struct rt_transport *transport)
{
    transport->socket_fd = -1;
    transport->connected = 0;
}

int rt_transport_connect(struct rt_transport *transport,
                         const char *host,
                         unsigned short port)
{
    (void)host;
    if (port == 0u) return -1;
    transport->socket_fd = 7;
    transport->connected = 1;
    return 0;
}

long rt_transport_send(struct rt_transport *transport,
                       const unsigned char *data,
                       size_t len)
{
    if (!transport->connected || fake_tx_len + len > sizeof(fake_tx)) return -1;
    memcpy(fake_tx + fake_tx_len, data, len);
    fake_tx_len += len;
    return (long)len;
}

long rt_transport_recv(struct rt_transport *transport,
                       unsigned char *data,
                       size_t len)
{
    size_t count;
    if (!transport->connected) return -1;
    if (fake_rx_len == 0u) return 0;
    count = fake_rx_len < len ? fake_rx_len : len;
    memcpy(data, fake_rx, count);
    fake_rx_len = 0u;
    return (long)count;
}

void rt_transport_disconnect(struct rt_transport *transport)
{
    transport->connected = 0;
    transport->socket_fd = -1;
    ++disconnect_count;
}

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

struct output_capture {
    unsigned char data[64];
    size_t len;
};

static void on_text(void *opaque, unsigned char byte)
{
    struct output_capture *capture = (struct output_capture *)opaque;
    if (capture->len < sizeof(capture->data)) capture->data[capture->len++] = byte;
}

static void on_control(void *opaque, unsigned char prefix,
                       unsigned char command,
                       const unsigned int *params, unsigned char count)
{
    (void)opaque; (void)prefix; (void)command; (void)params; (void)count;
}

static void on_app_data(void *opaque, const unsigned char *data, size_t len)
{
    struct output_capture *capture = (struct output_capture *)opaque;
    if (capture->len + len > sizeof(capture->data)) return;
    memcpy(capture->data + capture->len, data, len);
    capture->len += len;
}

static void reset_fake(void)
{
    fake_rx_len = 0u;
    fake_tx_len = 0u;
    disconnect_count = 0;
}

static void test_decoded_text_reaches_terminal(void)
{
    struct rt_app_session app;
    struct output_capture capture;
    const unsigned char input[] = {'H', 'i'};
    reset_fake(); memset(&capture, 0, sizeof(capture));
    rt_app_session_init(&app, 80u, 24u);
    require(rt_app_session_connect(&app, "example", 23u) == 0, "connect");
    memcpy(fake_rx, input, sizeof(input)); fake_rx_len = sizeof(input);
    require(rt_app_session_pump(&app, on_text, on_control, &capture) == 2, "pump text");
    require(capture.len == 2u && memcmp(capture.data, "Hi", 2u) == 0, "text data");
}

static void test_decoded_stream_observer(void)
{
    struct rt_app_session app;
    struct output_capture terminal_capture;
    struct output_capture data_capture;
    const unsigned char input[] = {'A', RT_IAC, RT_IAC, 'B'};
    const unsigned char expected[] = {'A', RT_IAC, 'B'};
    reset_fake(); memset(&terminal_capture, 0, sizeof(terminal_capture)); memset(&data_capture, 0, sizeof(data_capture));
    rt_app_session_init(&app, 80u, 24u);
    rt_app_session_set_data_observer(&app, on_app_data, &data_capture);
    require(rt_app_session_connect(&app, "example", 23u) == 0, "connect observer");
    memcpy(fake_rx, input, sizeof(input)); fake_rx_len = sizeof(input);
    require(rt_app_session_pump(&app, on_text, on_control, &terminal_capture) == 4, "pump observer");
    require(data_capture.len == sizeof(expected) && memcmp(data_capture.data, expected, sizeof(expected)) == 0, "observer decoded bytes");
}

static void test_do_naws_sends_will_and_dimensions(void)
{
    struct rt_app_session app;
    struct output_capture capture;
    const unsigned char input[] = {RT_IAC, RT_DO, RT_TELNET_OPT_NAWS};
    const unsigned char expected[] = {RT_IAC, RT_WILL, RT_TELNET_OPT_NAWS, RT_IAC, RT_SB, RT_TELNET_OPT_NAWS, 0u, 80u, 0u, 24u, RT_IAC, RT_SE};
    reset_fake(); memset(&capture, 0, sizeof(capture));
    rt_app_session_init(&app, 80u, 24u);
    require(rt_app_session_connect(&app, "example", 23u) == 0, "connect NAWS");
    memcpy(fake_rx, input, sizeof(input)); fake_rx_len = sizeof(input);
    require(rt_app_session_pump(&app, on_text, on_control, &capture) == 3, "pump NAWS");
    require(app.session.local_naws == 1u, "NAWS state");
    require(fake_tx_len == sizeof(expected) && memcmp(fake_tx, expected, sizeof(expected)) == 0, "NAWS reply and payload");
}

static void test_keyboard_iac_is_escaped(void)
{
    struct rt_app_session app;
    const unsigned char input[] = {'A', RT_IAC, 'B'};
    const unsigned char expected[] = {'A', RT_IAC, RT_IAC, 'B'};
    reset_fake(); rt_app_session_init(&app, 80u, 24u);
    require(rt_app_session_connect(&app, "example", 23u) == 0, "connect keyboard");
    require(rt_app_session_send_input(&app, input, sizeof(input)) == 4, "send keyboard");
    require(fake_tx_len == sizeof(expected) && memcmp(fake_tx, expected, sizeof(expected)) == 0, "keyboard IAC escape");
}

static void test_eof_disconnects_and_reconnect_resets(void)
{
    struct rt_app_session app;
    struct output_capture capture;
    reset_fake(); memset(&capture, 0, sizeof(capture));
    rt_app_session_init(&app, 80u, 24u);
    require(rt_app_session_connect(&app, "example", 23u) == 0, "connect EOF");
    app.session.remote_echo = 1u;
    require(rt_app_session_pump(&app, on_text, on_control, &capture) == 0, "EOF result");
    require(!app.transport.connected && app.transport.socket_fd < 0, "EOF disconnect");
    require(app.session.remote_echo == 0u, "EOF session reset");
    require(rt_app_session_connect(&app, "example", 2323u) == 0, "reconnect after EOF");
    require(app.transport.connected && app.session.remote_echo == 0u, "reconnect clean state");
}

int main(void)
{
    test_decoded_text_reaches_terminal();
    test_decoded_stream_observer();
    test_do_naws_sends_will_and_dimensions();
    test_keyboard_iac_is_escaped();
    test_eof_disconnects_and_reconnect_resets();
    puts("PASS: app session integration");
    return 0;
}
