#ifndef REXXTELNET_AREXX_CMD_H
#define REXXTELNET_AREXX_CMD_H

#include <stddef.h>

#define RT_AREXX_RC_OK 0
#define RT_AREXX_RC_WARN 5
#define RT_AREXX_RC_ERROR 10

#define RT_AREXX_ARG_MAX 256

struct rt_arexx_command {
    char name[16];
    char arg1[RT_AREXX_ARG_MAX];
    char arg2[RT_AREXX_ARG_MAX];
};

int rt_arexx_parse(const char *line, struct rt_arexx_command *out);

#endif
