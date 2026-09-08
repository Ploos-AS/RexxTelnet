#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "telnet.h"

struct capture {
    unsigned char data[64];
    size_t data_len;
    unsigned char verb;
    unsigned char option;
    int negotiations;
};

static void capture_data(void *ctx, unsigned char byte)
{
    struct capture *capture = (struct capture *)ctx;
    if (capture->data_len < sizeof(capture->data)) {
        capture->data[capture->data_len++] = byte;
    }
}

static void capture_negotiation(void *ctx,
                                unsigned char verb,
                                unsigned char option)
{
    struct capture *capture = (struct capture *)ctx;
    capture->verb = verb;
    capture->option = option;
    capture->negotiations++;
}

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void test_plain_data(void)
{
    struct rt_telnet_parser parser;
    struct capture capture;
    const unsigned char input[] = "hello";

    memset(&capture, 0, sizeof(capture));
    rt_telnet_init(&parser);
    rt_telnet_feed(&parser, input, 5u, capture_data,
                   capture_negotiation, &capture);

    require(capture.data_len == 5u, "plain data length");
    require(memcmp(capture.data, "hello", 5u) == 0, "plain data");
}

static void test_escaped_iac(void)
{
    struct rt_telnet_parser parser;
    struct capture capture;
    const unsigned char input[] = { 'A', RT_IAC, RT_IAC, 'B' };

    memset(&capture, 0, sizeof(capture));
    rt_telnet_init(&parser);
    rt_telnet_feed(&parser, input, sizeof(input), capture_data,
                   capture_negotiation, &capture);

    require(capture.data_len == 3u, "escaped IAC length");
    require(capture.data[0] == 'A', "escaped IAC prefix");
    require(capture.data[1] == RT_IAC, "escaped IAC byte");
    require(capture.data[2] == 'B', "escaped IAC suffix");
}

static void test_negotiation(void)
{
    struct rt_telnet_parser parser;
    struct capture capture;
    const unsigned char input[] = { RT_IAC, RT_WILL, 1u };

    memset(&capture, 0, sizeof(capture));
    rt_telnet_init(&parser);
    rt_telnet_feed(&parser, input, sizeof(input), capture_data,
                   capture_negotiation, &capture);

    require(capture.negotiations == 1, "negotiation count");
    require(capture.verb == RT_WILL, "negotiation verb");
    require(capture.option == 1u, "negotiation option");
}

static void test_subnegotiation_is_filtered(void)
{
    struct rt_telnet_parser parser;
    struct capture capture;
    const unsigned char input[] = {
        'X', RT_IAC, RT_SB, 31u, 0u, 80u, RT_IAC, RT_SE, 'Y'
    };

    memset(&capture, 0, sizeof(capture));
    rt_telnet_init(&parser);
    rt_telnet_feed(&parser, input, sizeof(input), capture_data,
                   capture_negotiation, &capture);

    require(capture.data_len == 2u, "subnegotiation filtered length");
    require(capture.data[0] == 'X', "subnegotiation prefix");
    require(capture.data[1] == 'Y', "subnegotiation suffix");
}

int main(void)
{
    test_plain_data();
    test_escaped_iac();
    test_negotiation();
    test_subnegotiation_is_filtered();
    puts("PASS: telnet parser");
    return 0;
}
