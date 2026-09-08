#ifndef REXXTELNET_APP_SESSION_H
#define REXXTELNET_APP_SESSION_H

#include "rx_buffer.h"
#include "session.h"
#include "terminal.h"
#include "transport.h"

struct rt_app_session {
    struct rt_transport transport;
    struct rt_session session;
    struct rt_terminal terminal;
    struct rt_rx_buffer rx_buffer;
};

void rt_app_session_init(struct rt_app_session *app,
                         unsigned short columns,
                         unsigned short rows);

int rt_app_session_connect(struct rt_app_session *app,
                           const char *host,
                           unsigned short port);

long rt_app_session_send_input(struct rt_app_session *app,
                               const unsigned char *data,
                               size_t len);

long rt_app_session_pump(struct rt_app_session *app,
                         rt_terminal_emit_cb on_text,
                         rt_terminal_control_cb on_control,
                         void *ctx);

void rt_app_session_disconnect(struct rt_app_session *app);

#endif
