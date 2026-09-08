#ifndef REXXTELNET_TERMINAL_H
#define REXXTELNET_TERMINAL_H

#include <stddef.h>

#define RT_TERM_MAX_PARAMS 4
#define RT_TERM_PREFIX_NONE 0u
#define RT_TERM_PREFIX_PRIVATE_QMARK ((unsigned char)'?')
#define RT_TERM_PREFIX_ESC 27u

struct rt_terminal {
    unsigned short columns;
    unsigned short rows;
    unsigned short cursor_x;
    unsigned short cursor_y;
    unsigned char esc_state;
    unsigned char csi_prefix;
    unsigned int params[RT_TERM_MAX_PARAMS];
    unsigned char param_count;
    unsigned int current_param;
    int current_param_set;
};

typedef void (*rt_terminal_emit_cb)(void *ctx, unsigned char byte);
typedef void (*rt_terminal_control_cb)(void *ctx,
                                       unsigned char prefix,
                                       unsigned char command,
                                       const unsigned int *params,
                                       unsigned char param_count);

void rt_terminal_init(struct rt_terminal *term,
                      unsigned short columns,
                      unsigned short rows);

void rt_terminal_feed(struct rt_terminal *term,
                      const unsigned char *data,
                      size_t len,
                      rt_terminal_emit_cb on_text,
                      rt_terminal_control_cb on_control,
                      void *ctx);

size_t rt_terminal_build_naws(unsigned short columns,
                              unsigned short rows,
                              unsigned char *output,
                              size_t output_size);

#endif
