#ifndef REXXTELNET_SESSION_IO_H
#define REXXTELNET_SESSION_IO_H

#include <stddef.h>

#include "session.h"
#include "transport.h"

struct rt_session_io {
    struct rt_session session;
    struct rt_transport transport;
};

typedef void (*rt_session_data_cb)(void *ctx,
                                   const unsigned char *data,
                                   size_t len);

void rt_session_io_init(struct rt_session_io *io);
int rt_session_io_connect(struct rt_session_io *io,
                          const char *host,
                          unsigned short port);
void rt_session_io_disconnect(struct rt_session_io *io);
long rt_session_io_send(struct rt_session_io *io,
                        const unsigned char *data,
                        size_t len);
long rt_session_io_pump(struct rt_session_io *io,
                        rt_session_data_cb on_data,
                        void *ctx);

#endif
