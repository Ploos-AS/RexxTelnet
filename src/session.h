#ifndef REXXTELNET_SESSION_H
#define REXXTELNET_SESSION_H

#include <stddef.h>

#include "telnet.h"

#define RT_TELNET_OPT_BINARY 0u
#define RT_TELNET_OPT_ECHO 1u
#define RT_TELNET_OPT_SGA 3u
#define RT_TELNET_OPT_NAWS 31u

struct rt_session {
    struct rt_telnet_parser parser;
    unsigned char local_binary;
    unsigned char remote_binary;
    unsigned char remote_echo;
    unsigned char local_sga;
    unsigned char remote_sga;
};

struct rt_reply {
    unsigned char bytes[3];
    size_t len;
};

void rt_session_init(struct rt_session *session);

struct rt_reply rt_session_negotiate(struct rt_session *session,
                                     unsigned char verb,
                                     unsigned char option);

size_t rt_telnet_encode_data(const unsigned char *input,
                             size_t input_len,
                             unsigned char *output,
                             size_t output_size);

#endif
