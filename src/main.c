/*
 * RexxTelnet Amiga application entry point.
 *
 * M0 intentionally keeps AmigaOS-specific networking, terminal and ARexx
 * plumbing outside the host-testable Telnet protocol core.
 */

#include "telnet.h"

int main(void)
{
    struct rt_telnet_parser parser;

    rt_telnet_init(&parser);

    /* M1 will add application lifecycle and bsdsocket.library integration. */
    return 0;
}
