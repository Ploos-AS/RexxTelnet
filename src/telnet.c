#include "telnet.h"

void rt_telnet_init(struct rt_telnet_parser *parser)
{
    if (parser != NULL) {
        parser->state = RT_TELNET_DATA;
        parser->verb = 0u;
    }
}

void rt_telnet_feed(struct rt_telnet_parser *parser,
                    const unsigned char *data,
                    size_t len,
                    rt_data_cb on_data,
                    rt_negotiation_cb on_negotiation,
                    void *ctx)
{
    size_t i;

    if (parser == NULL || data == NULL) {
        return;
    }

    for (i = 0u; i < len; ++i) {
        unsigned char byte = data[i];

        switch (parser->state) {
        case RT_TELNET_DATA:
            if (byte == RT_IAC) {
                parser->state = RT_TELNET_IAC;
            } else if (on_data != NULL) {
                on_data(ctx, byte);
            }
            break;

        case RT_TELNET_IAC:
            if (byte == RT_IAC) {
                if (on_data != NULL) {
                    on_data(ctx, byte);
                }
                parser->state = RT_TELNET_DATA;
            } else if (byte == RT_WILL || byte == RT_WONT ||
                       byte == RT_DO || byte == RT_DONT) {
                parser->verb = byte;
                parser->state = RT_TELNET_NEGOTIATION;
            } else if (byte == RT_SB) {
                parser->state = RT_TELNET_SB;
            } else {
                parser->state = RT_TELNET_DATA;
            }
            break;

        case RT_TELNET_NEGOTIATION:
            if (on_negotiation != NULL) {
                on_negotiation(ctx, parser->verb, byte);
            }
            parser->state = RT_TELNET_DATA;
            break;

        case RT_TELNET_SB:
            if (byte == RT_IAC) {
                parser->state = RT_TELNET_SB_IAC;
            }
            break;

        case RT_TELNET_SB_IAC:
            if (byte == RT_SE) {
                parser->state = RT_TELNET_DATA;
            } else if (byte != RT_IAC) {
                parser->state = RT_TELNET_SB;
            }
            break;
        }
    }
}
