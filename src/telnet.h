#ifndef REXXTELNET_TELNET_H
#define REXXTELNET_TELNET_H

#include <stddef.h>

#define RT_IAC  255u
#define RT_DONT 254u
#define RT_DO   253u
#define RT_WONT 252u
#define RT_WILL 251u
#define RT_SB   250u
#define RT_SE   240u

enum rt_telnet_state {
    RT_TELNET_DATA = 0,
    RT_TELNET_IAC,
    RT_TELNET_NEGOTIATION,
    RT_TELNET_SB,
    RT_TELNET_SB_IAC
};

struct rt_telnet_parser {
    enum rt_telnet_state state;
    unsigned char verb;
};

typedef void (*rt_data_cb)(void *ctx, unsigned char byte);
typedef void (*rt_negotiation_cb)(void *ctx,
                                  unsigned char verb,
                                  unsigned char option);

void rt_telnet_init(struct rt_telnet_parser *parser);

void rt_telnet_feed(struct rt_telnet_parser *parser,
                    const unsigned char *data,
                    size_t len,
                    rt_data_cb on_data,
                    rt_negotiation_cb on_negotiation,
                    void *ctx);

#endif
