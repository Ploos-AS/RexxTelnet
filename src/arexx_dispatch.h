#ifndef REXXTELNET_AREXX_DISPATCH_H
#define REXXTELNET_AREXX_DISPATCH_H

#include <stddef.h>

struct rt_arexx_ops {
    int (*is_connected)(void *ctx);
    int (*connect)(void *ctx, const char *host, unsigned short port);
    void (*disconnect)(void *ctx);
    long (*send)(void *ctx, const unsigned char *data, size_t len);
    unsigned short (*columns)(void *ctx);
    unsigned short (*rows)(void *ctx);
    int (*set_columns)(void *ctx, unsigned short value);
    int (*set_rows)(void *ctx, unsigned short value);
};

struct rt_arexx_result {
    int rc;
    int quit;
    char result[256];
};

void rt_arexx_dispatch(const char *line,
                       const struct rt_arexx_ops *ops,
                       void *ctx,
                       struct rt_arexx_result *out);

#endif
