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
    size_t (*peek)(void *ctx, char *output, size_t output_size);
    size_t (*read)(void *ctx, char *output, size_t output_size);
    int (*waitfor)(void *ctx, const char *text, unsigned long timeout_seconds);
    int (*capture_start)(void *ctx, const char *path);
    int (*capture_stop)(void *ctx);
    int (*get_property)(void *ctx, const char *name,
                        char *output, size_t output_size);
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
