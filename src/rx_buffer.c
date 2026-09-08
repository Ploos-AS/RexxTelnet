#include "rx_buffer.h"

#include <string.h>

void rt_rx_buffer_init(struct rt_rx_buffer *buffer)
{
    if (buffer != NULL) buffer->len = 0u;
}

void rt_rx_buffer_append(struct rt_rx_buffer *buffer,
                         const unsigned char *data,
                         size_t len)
{
    size_t drop;

    if (buffer == NULL || data == NULL || len == 0u) return;

    if (len >= RT_RX_BUFFER_SIZE) {
        memcpy(buffer->data,
               data + (len - RT_RX_BUFFER_SIZE),
               RT_RX_BUFFER_SIZE);
        buffer->len = RT_RX_BUFFER_SIZE;
        return;
    }

    if (buffer->len + len > RT_RX_BUFFER_SIZE) {
        drop = buffer->len + len - RT_RX_BUFFER_SIZE;
        memmove(buffer->data, buffer->data + drop, buffer->len - drop);
        buffer->len -= drop;
    }

    memcpy(buffer->data + buffer->len, data, len);
    buffer->len += len;
}

static size_t rt_rx_buffer_copy(const struct rt_rx_buffer *buffer,
                                char *output,
                                size_t output_size)
{
    size_t count;

    if (buffer == NULL || output == NULL || output_size == 0u) return 0u;
    count = buffer->len;
    if (count >= output_size) count = output_size - 1u;
    if (count != 0u) memcpy(output, buffer->data, count);
    output[count] = '\0';
    return count;
}

size_t rt_rx_buffer_peek(const struct rt_rx_buffer *buffer,
                         char *output,
                         size_t output_size)
{
    return rt_rx_buffer_copy(buffer, output, output_size);
}

size_t rt_rx_buffer_read(struct rt_rx_buffer *buffer,
                         char *output,
                         size_t output_size)
{
    size_t count;

    if (buffer == NULL) return 0u;
    count = rt_rx_buffer_copy(buffer, output, output_size);
    if (count != 0u) {
        memmove(buffer->data, buffer->data + count, buffer->len - count);
        buffer->len -= count;
    }
    return count;
}

static long rt_rx_buffer_find(const struct rt_rx_buffer *buffer,
                              const char *needle)
{
    size_t i;
    size_t needle_len;

    if (buffer == NULL || needle == NULL || *needle == '\0') return -1L;
    needle_len = strlen(needle);
    if (needle_len > buffer->len) return -1L;

    for (i = 0u; i + needle_len <= buffer->len; ++i) {
        if (memcmp(buffer->data + i, needle, needle_len) == 0)
            return (long)i;
    }
    return -1L;
}

int rt_rx_buffer_contains(const struct rt_rx_buffer *buffer,
                          const char *needle)
{
    return rt_rx_buffer_find(buffer, needle) >= 0L;
}

int rt_rx_buffer_consume_through(struct rt_rx_buffer *buffer,
                                 const char *needle)
{
    long found;
    size_t consume;
    size_t needle_len;

    if (buffer == NULL || needle == NULL) return 0;
    found = rt_rx_buffer_find(buffer, needle);
    if (found < 0L) return 0;

    needle_len = strlen(needle);
    consume = (size_t)found + needle_len;
    memmove(buffer->data, buffer->data + consume, buffer->len - consume);
    buffer->len -= consume;
    return 1;
}
