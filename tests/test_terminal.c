#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "terminal.h"
#include "telnet.h"

struct capture {
    unsigned char text[64];
    size_t text_len;
    unsigned char prefix;
    unsigned char command;
    unsigned int params[RT_TERM_MAX_PARAMS];
    unsigned char param_count;
};

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void capture_text(void *ctx, unsigned char byte)
{
    struct capture *capture = (struct capture *)ctx;
    if (capture->text_len < sizeof(capture->text))
        capture->text[capture->text_len++] = byte;
}

static void capture_control(void *ctx,
                            unsigned char prefix,
                            unsigned char command,
                            const unsigned int *params,
                            unsigned char param_count)
{
    struct capture *capture = (struct capture *)ctx;
    unsigned char i;
    capture->prefix = prefix;
    capture->command = command;
    capture->param_count = param_count;
    for (i = 0u; i < param_count && i < RT_TERM_MAX_PARAMS; ++i)
        capture->params[i] = params[i];
}

static void test_plain_text(void)
{
    struct rt_terminal term;
    struct capture capture;
    const unsigned char data[] = "hello";
    memset(&capture, 0, sizeof(capture));
    rt_terminal_init(&term, 80u, 24u);
    rt_terminal_feed(&term, data, 5u, capture_text, capture_control, &capture);
    require(capture.text_len == 5u, "plain text length");
    require(memcmp(capture.text, "hello", 5u) == 0, "plain text content");
}

static void test_fragmented_csi(void)
{
    struct rt_terminal term;
    struct capture capture;
    const unsigned char a[] = {27u, '[', '3'};
    const unsigned char b[] = {'1', ';', '4', '2', 'm'};
    memset(&capture, 0, sizeof(capture));
    rt_terminal_init(&term, 80u, 24u);
    rt_terminal_feed(&term, a, sizeof(a), capture_text, capture_control, &capture);
    rt_terminal_feed(&term, b, sizeof(b), capture_text, capture_control, &capture);
    require(capture.prefix == RT_TERM_PREFIX_NONE, "CSI prefix");
    require(capture.command == 'm', "CSI command");
    require(capture.param_count == 2u, "CSI param count");
    require(capture.params[0] == 31u && capture.params[1] == 42u,
            "CSI params");
}

static void test_private_csi(void)
{
    struct rt_terminal term;
    struct capture capture;
    const unsigned char data[] = {27u, '[', '?', '2', '5', 'l'};
    memset(&capture, 0, sizeof(capture));
    rt_terminal_init(&term, 80u, 24u);
    rt_terminal_feed(&term, data, sizeof(data), capture_text, capture_control, &capture);
    require(capture.prefix == RT_TERM_PREFIX_PRIVATE_QMARK, "private prefix");
    require(capture.command == 'l', "private command");
    require(capture.param_count == 1u && capture.params[0] == 25u,
            "private params");
}

static void test_non_csi_escape(void)
{
    struct rt_terminal term;
    struct capture capture;
    const unsigned char data[] = {27u, '7'};
    memset(&capture, 0, sizeof(capture));
    rt_terminal_init(&term, 80u, 24u);
    rt_terminal_feed(&term, data, sizeof(data), capture_text, capture_control, &capture);
    require(capture.prefix == RT_TERM_PREFIX_ESC, "ESC prefix");
    require(capture.command == '7', "ESC command");
    require(capture.param_count == 0u, "ESC no params");
}

static void test_naws(void)
{
    unsigned char out[16];
    size_t len = rt_terminal_build_naws(80u, 24u, out, sizeof(out));
    require(len == 9u, "NAWS length");
    require(out[0] == RT_IAC && out[1] == RT_SB && out[2] == 31u,
            "NAWS prefix");
    require(out[3] == 0u && out[4] == 80u && out[5] == 0u && out[6] == 24u,
            "NAWS dimensions");
    require(out[7] == RT_IAC && out[8] == RT_SE, "NAWS suffix");
}

static void test_naws_iac_escaping(void)
{
    unsigned char out[16];
    size_t len = rt_terminal_build_naws(255u, 24u, out, sizeof(out));
    require(len == 10u, "NAWS escaped length");
    require(out[4] == RT_IAC && out[5] == RT_IAC, "NAWS IAC escaped");
}

int main(void)
{
    test_plain_text();
    test_fragmented_csi();
    test_private_csi();
    test_non_csi_escape();
    test_naws();
    test_naws_iac_escaping();
    puts("PASS: terminal core");
    return 0;
}
