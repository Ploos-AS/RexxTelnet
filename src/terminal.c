#include "terminal.h"
#include "telnet.h"

#define RT_TERM_ESC 27u

static void rt_terminal_reset_csi(struct rt_terminal *term)
{
    unsigned char i;
    for (i = 0u; i < RT_TERM_MAX_PARAMS; ++i) term->params[i] = 0u;
    term->param_count = 0u;
    term->current_param = 0u;
    term->current_param_set = 0;
}

void rt_terminal_init(struct rt_terminal *term,
                      unsigned short columns,
                      unsigned short rows)
{
    if (term == NULL) return;
    term->columns = columns;
    term->rows = rows;
    term->cursor_x = 1u;
    term->cursor_y = 1u;
    term->esc_state = 0u;
    rt_terminal_reset_csi(term);
}

static void rt_terminal_finish_param(struct rt_terminal *term)
{
    if (term->param_count < RT_TERM_MAX_PARAMS) {
        term->params[term->param_count++] = term->current_param_set ?
            term->current_param : 0u;
    }
    term->current_param = 0u;
    term->current_param_set = 0;
}

void rt_terminal_feed(struct rt_terminal *term,
                      const unsigned char *data,
                      size_t len,
                      rt_terminal_emit_cb on_text,
                      rt_terminal_control_cb on_control,
                      void *ctx)
{
    size_t i;
    if (term == NULL || data == NULL) return;

    for (i = 0u; i < len; ++i) {
        unsigned char ch = data[i];

        if (term->esc_state == 0u) {
            if (ch == RT_TERM_ESC) {
                term->esc_state = 1u;
            } else {
                if (on_text != NULL) on_text(ctx, ch);
                if (ch == '\r') term->cursor_x = 1u;
                else if (ch == '\n') {
                    if (term->cursor_y < term->rows) term->cursor_y++;
                } else if (ch >= 32u && ch != 127u && term->cursor_x < term->columns) {
                    term->cursor_x++;
                }
            }
            continue;
        }

        if (term->esc_state == 1u) {
            if (ch == '[') {
                term->esc_state = 2u;
                rt_terminal_reset_csi(term);
            } else {
                term->esc_state = 0u;
            }
            continue;
        }

        if (ch >= '0' && ch <= '9') {
            term->current_param = term->current_param * 10u + (unsigned int)(ch - '0');
            term->current_param_set = 1;
        } else if (ch == ';') {
            rt_terminal_finish_param(term);
        } else if ((ch >= '@' && ch <= '~')) {
            if (term->current_param_set || term->param_count > 0u)
                rt_terminal_finish_param(term);
            if (on_control != NULL)
                on_control(ctx, ch, term->params, term->param_count);
            term->esc_state = 0u;
            rt_terminal_reset_csi(term);
        }
    }
}

static size_t rt_emit_telnet_byte(unsigned char byte,
                                  unsigned char *output,
                                  size_t output_size,
                                  size_t pos)
{
    if (pos >= output_size) return 0u;
    output[pos++] = byte;
    if (byte == RT_IAC) {
        if (pos >= output_size) return 0u;
        output[pos++] = RT_IAC;
    }
    return pos;
}

size_t rt_terminal_build_naws(unsigned short columns,
                              unsigned short rows,
                              unsigned char *output,
                              size_t output_size)
{
    unsigned char payload[4];
    size_t pos;
    size_t next;
    size_t i;

    if (output == NULL || output_size < 5u) return 0u;
    output[0] = RT_IAC;
    output[1] = RT_SB;
    output[2] = 31u;
    pos = 3u;

    payload[0] = (unsigned char)((columns >> 8) & 0xffu);
    payload[1] = (unsigned char)(columns & 0xffu);
    payload[2] = (unsigned char)((rows >> 8) & 0xffu);
    payload[3] = (unsigned char)(rows & 0xffu);

    for (i = 0u; i < 4u; ++i) {
        next = rt_emit_telnet_byte(payload[i], output, output_size, pos);
        if (next == 0u) return 0u;
        pos = next;
    }

    if (pos + 2u > output_size) return 0u;
    output[pos++] = RT_IAC;
    output[pos++] = RT_SE;
    return pos;
}
