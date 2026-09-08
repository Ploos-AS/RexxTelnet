#include "arexx_dispatch.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arexx_cmd.h"

static void set_result(struct rt_arexx_result *out, int rc, const char *text)
{
    out->rc = rc;
    out->quit = 0;
    if (text == NULL) text = "";
    strncpy(out->result, text, sizeof(out->result) - 1u);
    out->result[sizeof(out->result) - 1u] = '\0';
}

static int parse_ushort(const char *s, unsigned short *value)
{
    char *end;
    unsigned long v;
    if (s == NULL || *s == '\0') return 0;
    v = strtoul(s, &end, 10);
    if (*end != '\0' || v == 0u || v > 65535u) return 0;
    *value = (unsigned short)v;
    return 1;
}

static int text_equal_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

void rt_arexx_dispatch(const char *line,
                       const struct rt_arexx_ops *ops,
                       void *ctx,
                       struct rt_arexx_result *out)
{
    struct rt_arexx_command cmd;
    unsigned short value;
    char buffer[64];
    size_t len;

    if (out == NULL) return;
    set_result(out, RT_AREXX_RC_ERROR, "ERROR");
    if (ops == NULL || rt_arexx_parse(line, &cmd) != RT_AREXX_RC_OK) {
        set_result(out, RT_AREXX_RC_ERROR, "SYNTAX");
        return;
    }

    if (strcmp(cmd.name, "STATUS") == 0) {
        set_result(out, RT_AREXX_RC_OK,
                   ops->is_connected(ctx) ? "CONNECTED" : "DISCONNECTED");
    } else if (strcmp(cmd.name, "CONNECT") == 0) {
        unsigned short port = 23u;
        if (ops->is_connected(ctx)) {
            set_result(out, RT_AREXX_RC_WARN, "ALREADY CONNECTED");
            return;
        }
        if (cmd.arg1[0] == '\0' ||
            (cmd.arg2[0] != '\0' && !parse_ushort(cmd.arg2, &port))) {
            set_result(out, RT_AREXX_RC_ERROR, "SYNTAX");
            return;
        }
        if (ops->connect(ctx, cmd.arg1, port) == 0)
            set_result(out, RT_AREXX_RC_OK, "CONNECTED");
        else
            set_result(out, RT_AREXX_RC_ERROR, "CONNECT FAILED");
    } else if (strcmp(cmd.name, "DISCONNECT") == 0) {
        if (!ops->is_connected(ctx)) {
            set_result(out, RT_AREXX_RC_WARN, "NOT CONNECTED");
            return;
        }
        ops->disconnect(ctx);
        set_result(out, RT_AREXX_RC_OK, "DISCONNECTED");
    } else if (strcmp(cmd.name, "SEND") == 0 ||
               strcmp(cmd.name, "SENDLINE") == 0) {
        unsigned char sendbuf[(RT_AREXX_ARG_MAX * 2) + 3];
        if (!ops->is_connected(ctx)) {
            set_result(out, RT_AREXX_RC_WARN, "NOT CONNECTED");
            return;
        }
        len = strlen(cmd.arg1);
        memcpy(sendbuf, cmd.arg1, len);
        if (cmd.arg2[0] != '\0') {
            sendbuf[len++] = ' ';
            memcpy(sendbuf + len, cmd.arg2, strlen(cmd.arg2));
            len += strlen(cmd.arg2);
        }
        if (strcmp(cmd.name, "SENDLINE") == 0) {
            sendbuf[len++] = '\r';
            sendbuf[len++] = '\n';
        }
        if (ops->send(ctx, sendbuf, len) >= 0)
            set_result(out, RT_AREXX_RC_OK, "OK");
        else
            set_result(out, RT_AREXX_RC_ERROR, "SEND FAILED");
    } else if (strcmp(cmd.name, "GET") == 0) {
        if (text_equal_ci(cmd.arg1, "COLUMNS"))
            sprintf(buffer, "%u", (unsigned int)ops->columns(ctx));
        else if (text_equal_ci(cmd.arg1, "ROWS"))
            sprintf(buffer, "%u", (unsigned int)ops->rows(ctx));
        else {
            set_result(out, RT_AREXX_RC_ERROR, "UNKNOWN PROPERTY");
            return;
        }
        set_result(out, RT_AREXX_RC_OK, buffer);
    } else if (strcmp(cmd.name, "SET") == 0) {
        if (!parse_ushort(cmd.arg2, &value)) {
            set_result(out, RT_AREXX_RC_ERROR, "SYNTAX");
            return;
        }
        if (text_equal_ci(cmd.arg1, "COLUMNS")) {
            if (ops->set_columns(ctx, value) == 0)
                set_result(out, RT_AREXX_RC_OK, "OK");
            else
                set_result(out, RT_AREXX_RC_ERROR, "SET FAILED");
        } else if (text_equal_ci(cmd.arg1, "ROWS")) {
            if (ops->set_rows(ctx, value) == 0)
                set_result(out, RT_AREXX_RC_OK, "OK");
            else
                set_result(out, RT_AREXX_RC_ERROR, "SET FAILED");
        } else {
            set_result(out, RT_AREXX_RC_ERROR, "UNKNOWN PROPERTY");
        }
    } else if (strcmp(cmd.name, "QUIT") == 0) {
        set_result(out, RT_AREXX_RC_OK, "BYE");
        out->quit = 1;
    } else {
        set_result(out, RT_AREXX_RC_ERROR, "UNKNOWN COMMAND");
    }
}
