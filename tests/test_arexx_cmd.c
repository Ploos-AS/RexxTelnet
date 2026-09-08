#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arexx_cmd.h"

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(void)
{
    struct rt_arexx_command cmd;

    require(rt_arexx_parse("status", &cmd) == 0, "parse status");
    require(strcmp(cmd.name, "STATUS") == 0, "uppercase name");

    require(rt_arexx_parse("CONNECT bbs.example.org 2323", &cmd) == 0,
            "parse connect");
    require(strcmp(cmd.name, "CONNECT") == 0, "connect name");
    require(strcmp(cmd.arg1, "bbs.example.org") == 0, "connect host");
    require(strcmp(cmd.arg2, "2323") == 0, "connect port");

    require(rt_arexx_parse("SENDLINE hello world", &cmd) == 0,
            "parse sendline");
    require(strcmp(cmd.arg1, "hello") == 0, "sendline first token");
    require(strcmp(cmd.arg2, "world") == 0, "sendline tail");

    require(rt_arexx_parse("WAITFOR \"login prompt:\" 10", &cmd) == 0,
            "parse quoted waitfor");
    require(strcmp(cmd.arg1, "login prompt:") == 0, "quoted waitfor text");
    require(strcmp(cmd.arg2, "10") == 0, "quoted waitfor timeout");

    require(rt_arexx_parse("WAITFOR \"unterminated", &cmd) == RT_AREXX_RC_ERROR,
            "unterminated quote rejected");

    require(rt_arexx_parse("   ", &cmd) == RT_AREXX_RC_ERROR,
            "blank rejected");

    puts("PASS: arexx command parser");
    return 0;
}
