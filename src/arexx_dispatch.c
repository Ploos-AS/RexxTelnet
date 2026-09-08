#include "arexx_dispatch.h"

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
        if (ops->is_connected(ctx)) { set_result(out, RT_AREXX_RC_WARN, "ALREADY CONNECTED"); return; }
        if (cmd.arg1[0] == '\0' || (cmd.arg2[0] != '\0' && !parse_ushort(cmd.arg2, &port))) {
            set_result(out, RT_AREXX_RC_ERROR, "SYNTAX"); return;
        }
        set_result(out, ops->connect(ctx, cmd.arg1, port) == 0 ? RT_AREXX_RC_OK : RT_AREXX_RC_ERROR,
                   ops->is_connected(ctx) ? "CONNECTED" : "CONNECT FAILED");
    } else if (strcmp(cmd.name, "DISCONNECT") == 0) {
        if (!ops->is_connected(ctx)) { set_result(out, RT_AREXX_RC_WARN, "NOT CONNECTED"); return; }
        ops->disconnect(ctx); set_result(out, RT_AREXX_RC_OK, "DISCONNECTED");
    } else if (strcmp(cmd.name, "SEND") == 0 || strcmp(cmd.name, "SENDLINE") == 0) {
        unsigned char sendbuf[RT_AREXX_ARG_MAX + 2];
        if (!ops->is_connected(ctx)) { set_result(out, RT_AREXX_RC_WARN, "NOT CONNECTED"); return; }
        len = strlen(cmd.arg1);
        memcpy(sendbuf, cmd.arg1, len);
        if (strcmp(cmd.name, "SENDLINE") == 0) { sendbuf[len++] = '\r'; sendbuf[len++] = '\n'; }
        set_result(out, ops->send(ctx, sendbuf, len) >= 0 ? RT_AREXX_RC_OK : RT_AREXX_RC_ERROR,
                   "OK");
    } else if (strcmp(cmd.name, "GET") == 0) {
        if (strcmp(cmd.arg1, "COLUMNS") == 0) sprintf(buffer, "%u", (unsigned int)ops->columns(ctx));
        else if (strcmp(cmd.arg1, "ROWS") == 0) sprintf(buffer, "%u", (unsigned int)ops->rows(ctx));
        else { set_result(out, RT_AREXX_RC_ERROR, "UNKNOWN PROPERTY"); return; }
        set_result(out, RT_AREXX_RC_OK, buffer);
    } else if (strcmp(cmd.name, "SET") == 0) {
        if (!parse_ushort(cmd.arg2, &value)) { set_result(out, RT_AREXX_RC_ERROR, "SYNTAX"); return; }
        if (strcmp(cmd.arg1, "COLUMNS") == 0) set_result(out, ops->set_columns(ctx, value) == 0 ? 0 : 10, "OK");
        else if (strcmp(cmd.arg1, "ROWS") == 0) set_result(out, ops->set_rows(ctx, value) == 0 ? 0 : 10, "OK");
        else set_result(out, RT_AREXX_RC_ERROR, "UNKNOWN PROPERTY");
    } else if (strcmp(cmd.name, "QUIT") == 0) {
        set_result(out, RT_AREXX_RC_OK, "BYE"); out->quit = 1;
    } else {
        set_result(out, RT_AREXX_RC_ERROR, "UNKNOWN COMMAND");
    }
}
