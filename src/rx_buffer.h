#ifndef REXXTELNET_RX_BUFFER_H
#define REXXTELNET_RX_BUFFER_H

#include <stddef.h>

#define RT_RX_BUFFER_SIZE 4096u

struct rt_rx_buffer {
    unsigned char data[RT_RX_BUFFER_SIZE];
    size_t len;
};

void rt_rx_buffer_init(struct rt_rx_buffer *buffer);
void rt_rx_buffer_append(struct rt_rx_buffer *buffer,
                         const unsigned char *data,
                         size_t len);
size_t rt_rx_buffer_peek(const struct rt_rx_buffer *buffer,
                         char *output,
                         size_t output_size);
size_t rt_rx_buffer_read(struct rt_rx_buffer *buffer,
                         char *output,
                         size_t output_size);
int rt_rx_buffer_contains(const struct rt_rx_buffer *buffer,
                          const char *needle);
int rt_rx_buffer_consume_through(struct rt_rx_buffer *buffer,
                                 const char *needle);

#endif
