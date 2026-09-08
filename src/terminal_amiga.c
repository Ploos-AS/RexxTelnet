#include "terminal.h"
#include "terminal_amiga.h"

#ifdef __AMIGA__

#include <exec/types.h>
#include <proto/dos.h>
#include <dos/dos.h>

static BPTR rt_console = 0;

int rt_terminal_amiga_open(void)
{
    if (rt_console != 0) return 0;
    rt_console = Open((CONST_STRPTR)"CON:0/0/640/200/RexxTelnet/CLOSE/WAIT",
                      MODE_OLDFILE);
    return rt_console != 0 ? 0 : -1;
}

int rt_terminal_amiga_has_input(void)
{
    if (rt_console == 0) return 0;
    return WaitForChar(rt_console, 0L) ? 1 : 0;
}

long rt_terminal_amiga_write(const unsigned char *data, unsigned long len)
{
    if (rt_console == 0 || data == NULL) return -1;
    return (long)Write(rt_console, (APTR)data, len);
}

long rt_terminal_amiga_read(unsigned char *data, unsigned long len)
{
    if (rt_console == 0 || data == NULL) return -1;
    return (long)Read(rt_console, data, len);
}

void rt_terminal_amiga_close(void)
{
    if (rt_console != 0) {
        Close(rt_console);
        rt_console = 0;
    }
}

#endif
