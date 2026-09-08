#include "arexx_cmd.h"

#include <ctype.h>
#include <string.h>

static void copy_upper(char *dst, size_t dst_size, const char *src, size_t len)
{
    size_t i;
    if (dst_size == 0u) return;
    if (len >= dst_size) len = dst_size - 1u;
    for (i = 0u; i < len; ++i) dst[i] = (char)toupper((unsigned char)src[i]);
    dst[len] = '\0';
}

static void copy_arg(char *dst, size_t dst_size, const char *src, size_t len)
{
    if (dst_size == 0u) return;
    if (len >= dst_size) len = dst_size - 1u;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

int rt_arexx_parse(const char *line, struct rt_arexx_command *out)
{
    const char *p;
    const char *start;
    size_t len;

    if (line == NULL || out == NULL) return RT_AREXX_RC_ERROR;
    memset(out, 0, sizeof(*out));

    p = line;
    while (*p != '\0' && isspace((unsigned char)*p)) ++p;
    if (*p == '\0') return RT_AREXX_RC_ERROR;

    start = p;
    while (*p != '\0' && !isspace((unsigned char)*p)) ++p;
    len = (size_t)(p - start);
    if (len == 0u || len >= sizeof(out->name)) return RT_AREXX_RC_ERROR;
    copy_upper(out->name, sizeof(out->name), start, len);

    while (*p != '\0' && isspace((unsigned char)*p)) ++p;
    if (*p == '\0') return RT_AREXX_RC_OK;

    if (*p == '"') {
        ++p;
        start = p;
        while (*p != '\0' && *p != '"') ++p;
        if (*p != '"') return RT_AREXX_RC_ERROR;
        copy_arg(out->arg1, sizeof(out->arg1), start, (size_t)(p - start));
        ++p;
    } else {
        start = p;
        while (*p != '\0' && !isspace((unsigned char)*p)) ++p;
        copy_arg(out->arg1, sizeof(out->arg1), start, (size_t)(p - start));
    }

    while (*p != '\0' && isspace((unsigned char)*p)) ++p;
    if (*p != '\0') copy_arg(out->arg2, sizeof(out->arg2), p, strlen(p));

    return RT_AREXX_RC_OK;
}
