#include <stdio.h>
#include <stdlib.h>

#include "session.h"

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void require_reply(struct rt_reply reply,
                          unsigned char verb,
                          unsigned char option,
                          const char *message)
{
    require(reply.len == 3u, message);
    require(reply.bytes[0] == RT_IAC, message);
    require(reply.bytes[1] == verb, message);
    require(reply.bytes[2] == option, message);
}

static void test_supported_remote_options(void)
{
    struct rt_session session;
    struct rt_reply reply;
    rt_session_init(&session);

    reply = rt_session_negotiate(&session, RT_WILL, RT_TELNET_OPT_ECHO);
    require_reply(reply, RT_DO, RT_TELNET_OPT_ECHO, "accept remote ECHO");
    require(session.remote_echo == 1u, "remote ECHO state");
    reply = rt_session_negotiate(&session, RT_WILL, RT_TELNET_OPT_ECHO);
    require(reply.len == 0u, "suppress duplicate remote ECHO");

    reply = rt_session_negotiate(&session, RT_WILL, RT_TELNET_OPT_BINARY);
    require_reply(reply, RT_DO, RT_TELNET_OPT_BINARY, "accept remote BINARY");
    require(session.remote_binary == 1u, "remote BINARY state");
}

static void test_supported_local_options(void)
{
    struct rt_session session;
    struct rt_reply reply;
    rt_session_init(&session);

    reply = rt_session_negotiate(&session, RT_DO, RT_TELNET_OPT_SGA);
    require_reply(reply, RT_WILL, RT_TELNET_OPT_SGA, "accept local SGA");
    require(session.local_sga == 1u, "local SGA state");
    reply = rt_session_negotiate(&session, RT_DO, RT_TELNET_OPT_SGA);
    require(reply.len == 0u, "suppress duplicate local SGA");

    reply = rt_session_negotiate(&session, RT_DO, RT_TELNET_OPT_NAWS);
    require_reply(reply, RT_WILL, RT_TELNET_OPT_NAWS, "accept local NAWS");
    reply = rt_session_negotiate(&session, RT_DO, RT_TELNET_OPT_NAWS);
    require(reply.len == 0u, "suppress duplicate local NAWS");
}

static void test_unknown_options_are_rejected(void)
{
    struct rt_session session;
    struct rt_reply reply;
    rt_session_init(&session);

    reply = rt_session_negotiate(&session, RT_WILL, 42u);
    require_reply(reply, RT_DONT, 42u, "reject remote unknown option");

    reply = rt_session_negotiate(&session, RT_DO, 42u);
    require_reply(reply, RT_WONT, 42u, "reject local unknown option");
}

static void test_iac_encoding(void)
{
    const unsigned char input[] = {'A', RT_IAC, 'B'};
    unsigned char output[4];
    size_t len = rt_telnet_encode_data(input, sizeof(input), output,
                                       sizeof(output));
    require(len == 4u, "encoded length");
    require(output[0] == 'A', "encoded prefix");
    require(output[1] == RT_IAC && output[2] == RT_IAC,
            "IAC doubled");
    require(output[3] == 'B', "encoded suffix");
}

int main(void)
{
    test_supported_remote_options();
    test_supported_local_options();
    test_unknown_options_are_rejected();
    test_iac_encoding();
    puts("PASS: session policy");
    return 0;
}
