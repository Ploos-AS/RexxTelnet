#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "telnet.h"

struct capture {
    unsigned char data[16];
    size_t len;
    int negotiations;
    unsigned char verb;
    unsigned char option;
};

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void on_data(void *opaque, unsigned char byte)
{
    struct capture *capture = (struct capture *)opaque;
    capture->data[capture->len++] = byte;
}

static void on_neg(void *opaque, unsigned char verb, unsigned char option)
{
    struct capture *capture = (struct capture *)opaque;
    capture->negotiations++;
    capture->verb = verb;
    capture->option = option;
}

static void test_fragmented_negotiation(void)
{
    struct rt_telnet_parser parser;
    struct capture capture;
    const unsigned char a[] = {'A', RT_IAC};
    const unsigned char b[] = {RT_WILL};
    const unsigned char c[] = {1u, 'B'};

    memset(&capture, 0, sizeof(capture));
    rt_telnet_init(&parser);
    rt_telnet_feed(&parser, a, sizeof(a), on_data, on_neg, &capture);
    rt_telnet_feed(&parser, b, sizeof(b), on_data, on_neg, &capture);
    rt_telnet_feed(&parser, c, sizeof(c), on_data, on_neg, &capture);

    require(capture.len == 2u, "fragment data length");
    require(capture.data[0] == 'A' && capture.data[1] == 'B',
            "fragment data preserved");
    require(capture.negotiations == 1, "fragment negotiation count");
    require(capture.verb == RT_WILL && capture.option == 1u,
            "fragment negotiation value");
}

static void test_fragmented_escaped_iac(void)
{
    struct rt_telnet_parser parser;
    struct capture capture;
    const unsigned char a[] = {RT_IAC};
    const unsigned char b[] = {RT_IAC};

    memset(&capture, 0, sizeof(capture));
    rt_telnet_init(&parser);
    rt_telnet_feed(&parser, a, sizeof(a), on_data, on_neg, &capture);
    rt_telnet_feed(&parser, b, sizeof(b), on_data, on_neg, &capture);

    require(capture.len == 1u && capture.data[0] == RT_IAC,
            "fragment escaped IAC");
}

int main(void)
{
    test_fragmented_negotiation();
    test_fragmented_escaped_iac();
    puts("PASS: fragmentation");
    return 0;
}
