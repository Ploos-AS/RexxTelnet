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

static int equals_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
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

static int parse_timeout(const char *s, unsigned long *value)
{
    char *end;
    unsigned long v;
    if (s == NULL || *s == '\0') { *value = 10u; return 1; }
    v = strtoul(s, &end, 10);
    if (*end != '\0' || v > 3600u) return 0;
    *value = v;
    return 1;
}

static size_t build_send_text(const struct rt_arexx_command *cmd,
                              unsigned char *output,
                              size_t output_size)
{
    size_t len1 = strlen(cmd->arg1);
    size_t len2 = strlen(cmd->arg2);
    size_t total = len1 + (len2 != 0u ? 1u + len2 : 0u);

    if (total > output_size) return 0u;
    if (len1 != 0u) memcpy(output, cmd->arg1, len1);
    if (len2 != 0u) {
        output[len1] = ' ';
        memcpy(output + len1 + 1u, cmd->arg2, len2);
    }
    return total;
}

static int hex_value(unsigned char ch)
{
    if (ch >= '0' && ch <= '9') return (int)(ch - '0');
    ch = (unsigned char)toupper(ch);
    if (ch >= 'A' && ch <= 'F') return 10 + (int)(ch - 'A');
    return -1;
}

static size_t parse_hex_bytes(const struct rt_arexx_command *cmd,
                              unsigned char *output,
                              size_t output_size)
{
    char text[(RT_AREXX_ARG_MAX * 2) + 2];
    size_t text_len;
    size_t i;
    size_t out_len;
    int high;

    text_len = build_send_text(cmd, (unsigned char *)text, sizeof(text) - 1u);
    if (text_len == 0u) return 0u;
    text[text_len] = '\0';
    i = 0u;
    out_len = 0u;

    while (i < text_len) {
        while (i < text_len && (isspace((unsigned char)text[i]) ||
               text[i] == ':' || text[i] == '-')) ++i;
        if (i >= text_len) break;
        if (i + 1u < text_len && text[i] == '0' &&
            (text[i + 1u] == 'x' || text[i + 1u] == 'X')) i += 2u;
        if (i >= text_len) return 0u;
        high = hex_value((unsigned char)text[i++]);
        if (high < 0 || i >= text_len) return 0u;
        {
            int low = hex_value((unsigned char)text[i++]);
            if (low < 0 || out_len >= output_size) return 0u;
            output[out_len++] = (unsigned char)((high << 4) | low);
        }
        if (i < text_len && !isspace((unsigned char)text[i]) &&
            text[i] != ':' && text[i] != '-' &&
            hex_value((unsigned char)text[i]) < 0) return 0u;
    }
    return out_len;
}

void rt_arexx_dispatch(const char *line,
                       const struct rt_arexx_ops *ops,
                       void *ctx,
                       struct rt_arexx_result *out)
{
    struct rt_arexx_command cmd;
    unsigned short value;
    char buffer[256];
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
        unsigned char sendbuf[(RT_AREXX_ARG_MAX * 2) + 4];
        if (!ops->is_connected(ctx)) { set_result(out, RT_AREXX_RC_WARN, "NOT CONNECTED"); return; }
        len = build_send_text(&cmd, sendbuf, sizeof(sendbuf) - 2u);
        if (len == 0u && cmd.arg1[0] != '\0') { set_result(out, RT_AREXX_RC_ERROR, "TOO LONG"); return; }
        if (strcmp(cmd.name, "SENDLINE") == 0) { sendbuf[len++] = '\r'; sendbuf[len++] = '\n'; }
        set_result(out, ops->send(ctx, sendbuf, len) >= 0 ? RT_AREXX_RC_OK : RT_AREXX_RC_ERROR,
                   "OK");
    } else if (strcmp(cmd.name, "SENDHEX") == 0) {
        unsigned char sendbuf[RT_AREXX_ARG_MAX];
        if (!ops->is_connected(ctx)) { set_result(out, RT_AREXX_RC_WARN, "NOT CONNECTED"); return; }
        len = parse_hex_bytes(&cmd, sendbuf, sizeof(sendbuf));
        if (len == 0u) { set_result(out, RT_AREXX_RC_ERROR, "SYNTAX"); return; }
        set_result(out, ops->send(ctx, sendbuf, len) >= 0 ? RT_AREXX_RC_OK : RT_AREXX_RC_ERROR,
                   "OK");
    } else if (strcmp(cmd.name, "READ") == 0) {
        if (ops->read == NULL) { set_result(out, RT_AREXX_RC_ERROR, "UNAVAILABLE"); return; }
        ops->read(ctx, buffer, sizeof(buffer));
        set_result(out, RT_AREXX_RC_OK, buffer);
    } else if (strcmp(cmd.name, "PEEK") == 0) {
        if (ops->peek == NULL) { set_result(out, RT_AREXX_RC_ERROR, "UNAVAILABLE"); return; }
        ops->peek(ctx, buffer, sizeof(buffer));
        set_result(out, RT_AREXX_RC_OK, buffer);
    } else if (strcmp(cmd.name, "WAITFOR") == 0) {
        unsigned long timeout;
        int wait_rc;
        if (!ops->is_connected(ctx)) { set_result(out, RT_AREXX_RC_WARN, "NOT CONNECTED"); return; }
        if (cmd.arg1[0] == '\0' || !parse_timeout(cmd.arg2, &timeout) || ops->waitfor == NULL) {
            set_result(out, RT_AREXX_RC_ERROR, "SYNTAX"); return;
        }
        wait_rc = ops->waitfor(ctx, cmd.arg1, timeout);
        if (wait_rc > 0) set_result(out, RT_AREXX_RC_OK, "MATCH");
        else if (wait_rc == 0) set_result(out, RT_AREXX_RC_WARN, "TIMEOUT");
        else set_result(out, RT_AREXX_RC_ERROR, "WAIT FAILED");
    } else if (strcmp(cmd.name, "CAPTURE") == 0) {
        if (equals_ci(cmd.arg1, "STOP") && cmd.arg2[0] == '\0') {
            if (ops->capture_stop == NULL) { set_result(out, RT_AREXX_RC_ERROR, "UNAVAILABLE"); return; }
            set_result(out, ops->capture_stop(ctx) == 0 ? RT_AREXX_RC_OK : RT_AREXX_RC_WARN,
                       "CAPTURE STOPPED");
        } else {
            if (cmd.arg1[0] == '\0' || cmd.arg2[0] != '\0' || ops->capture_start == NULL) {
                set_result(out, RT_AREXX_RC_ERROR, "SYNTAX"); return;
            }
            set_result(out, ops->capture_start(ctx, cmd.arg1) == 0 ? RT_AREXX_RC_OK : RT_AREXX_RC_ERROR,
                       "CAPTURE STARTED");
        }
    } else if (strcmp(cmd.name, "GET") == 0) {
        if (equals_ci(cmd.arg1, "COLUMNS")) sprintf(buffer, "%u", (unsigned int)ops->columns(ctx));
        else if (equals_ci(cmd.arg1, "ROWS")) sprintf(buffer, "%u", (unsigned int)ops->rows(ctx));
        else if (equals_ci(cmd.arg1, "CONNECTED")) strcpy(buffer, ops->is_connected(ctx) ? "1" : "0");
        else if (ops->get_property != NULL && ops->get_property(ctx, cmd.arg1, buffer, sizeof(buffer)) == 0) {
            /* provider filled buffer */
        } else { set_result(out, RT_AREXX_RC_ERROR, "UNKNOWN PROPERTY"); return; }
        set_result(out, RT_AREXX_RC_OK, buffer);
    } else if (strcmp(cmd.name, "SET") == 0) {
        if (!parse_ushort(cmd.arg2, &value)) { set_result(out, RT_AREXX_RC_ERROR, "SYNTAX"); return; }
        if (equals_ci(cmd.arg1, "COLUMNS")) set_result(out, ops->set_columns(ctx, value) == 0 ? 0 : 10, "OK");
        else if (equals_ci(cmd.arg1, "ROWS")) set_result(out, ops->set_rows(ctx, value) == 0 ? 0 : 10, "OK");
        else set_result(out, RT_AREXX_RC_ERROR, "UNKNOWN PROPERTY");
    } else if (strcmp(cmd.name, "QUIT") == 0) {
        set_result(out, RT_AREXX_RC_OK, "BYE"); out->quit = 1;
    } else {
        set_result(out, RT_AREXX_RC_ERROR, "UNKNOWN COMMAND");
    }
}
