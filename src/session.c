#include "session.h"

static struct rt_reply rt_reply3(unsigned char verb, unsigned char option)
{
    struct rt_reply reply;
    reply.bytes[0] = RT_IAC;
    reply.bytes[1] = verb;
    reply.bytes[2] = option;
    reply.len = 3u;
    return reply;
}

void rt_session_init(struct rt_session *session)
{
    if (session != NULL) {
        rt_telnet_init(&session->parser);
        session->local_binary = 0u;
        session->remote_binary = 0u;
        session->remote_echo = 0u;
        session->local_sga = 0u;
        session->remote_sga = 0u;
        session->local_naws = 0u;
    }
}

struct rt_reply rt_session_negotiate(struct rt_session *session,
                                     unsigned char verb,
                                     unsigned char option)
{
    struct rt_reply none = {{0u, 0u, 0u}, 0u};

    if (session == NULL) return none;

    if (verb == RT_WILL) {
        if (option == RT_TELNET_OPT_BINARY) {
            if (session->remote_binary) return none;
            session->remote_binary = 1u;
            return rt_reply3(RT_DO, option);
        }
        if (option == RT_TELNET_OPT_ECHO) {
            if (session->remote_echo) return none;
            session->remote_echo = 1u;
            return rt_reply3(RT_DO, option);
        }
        if (option == RT_TELNET_OPT_SGA) {
            if (session->remote_sga) return none;
            session->remote_sga = 1u;
            return rt_reply3(RT_DO, option);
        }
        return rt_reply3(RT_DONT, option);
    }

    if (verb == RT_WONT) {
        if (option == RT_TELNET_OPT_BINARY) session->remote_binary = 0u;
        if (option == RT_TELNET_OPT_ECHO) session->remote_echo = 0u;
        if (option == RT_TELNET_OPT_SGA) session->remote_sga = 0u;
        return none;
    }

    if (verb == RT_DO) {
        if (option == RT_TELNET_OPT_BINARY) {
            if (session->local_binary) return none;
            session->local_binary = 1u;
            return rt_reply3(RT_WILL, option);
        }
        if (option == RT_TELNET_OPT_SGA) {
            if (session->local_sga) return none;
            session->local_sga = 1u;
            return rt_reply3(RT_WILL, option);
        }
        if (option == RT_TELNET_OPT_NAWS) {
            if (session->local_naws) return none;
            session->local_naws = 1u;
            return rt_reply3(RT_WILL, option);
        }
        return rt_reply3(RT_WONT, option);
    }

    if (verb == RT_DONT) {
        if (option == RT_TELNET_OPT_BINARY) session->local_binary = 0u;
        if (option == RT_TELNET_OPT_SGA) session->local_sga = 0u;
        if (option == RT_TELNET_OPT_NAWS) session->local_naws = 0u;
        return none;
    }

    return none;
}

size_t rt_telnet_encode_data(const unsigned char *input,
                             size_t input_len,
                             unsigned char *output,
                             size_t output_size)
{
    size_t i;
    size_t out = 0u;

    if (input == NULL || output == NULL) return 0u;

    for (i = 0u; i < input_len; ++i) {
        size_t needed = input[i] == RT_IAC ? 2u : 1u;
        if (out + needed > output_size) return 0u;
        output[out++] = input[i];
        if (input[i] == RT_IAC) output[out++] = RT_IAC;
    }
    return out;
}
