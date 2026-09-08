#include "arexx_amiga.h"

#ifdef __AMIGA__

#include <exec/ports.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <stdlib.h>
#include <string.h>

struct Library *RexxSysBase = NULL;

struct rt_arexx_port {
    struct MsgPort *port;
};

struct rt_arexx_port *rt_arexx_port_open(void)
{
    struct rt_arexx_port *wrapper;
    struct MsgPort *port;

    if (FindPort("REXXTELNET") != NULL) return NULL;

    RexxSysBase = OpenLibrary("rexxsyslib.library", 0);
    if (RexxSysBase == NULL) return NULL;

    wrapper = (struct rt_arexx_port *)malloc(sizeof(*wrapper));
    if (wrapper == NULL) { CloseLibrary(RexxSysBase); RexxSysBase = NULL; return NULL; }

    port = CreateMsgPort();
    if (port == NULL) {
        free(wrapper);
        CloseLibrary(RexxSysBase); RexxSysBase = NULL;
        return NULL;
    }

    port->mp_Node.ln_Name = "REXXTELNET";
    port->mp_Node.ln_Pri = 0;
    port->mp_Node.ln_Type = NT_MSGPORT;
    AddPort(port);
    wrapper->port = port;
    return wrapper;
}

void rt_arexx_port_close(struct rt_arexx_port *wrapper)
{
    if (wrapper == NULL) return;
    if (wrapper->port != NULL) {
        RemPort(wrapper->port);
        DeleteMsgPort(wrapper->port);
    }
    free(wrapper);
    if (RexxSysBase != NULL) { CloseLibrary(RexxSysBase); RexxSysBase = NULL; }
}

unsigned long rt_arexx_port_signal_mask(const struct rt_arexx_port *wrapper)
{
    if (wrapper == NULL || wrapper->port == NULL) return 0u;
    return 1UL << wrapper->port->mp_SigBit;
}

int rt_arexx_port_pending(const struct rt_arexx_port *wrapper)
{
    if (wrapper == NULL || wrapper->port == NULL) return 0;
    return wrapper->port->mp_MsgList.lh_Head->ln_Succ != NULL;
}

int rt_arexx_port_process(struct rt_arexx_port *wrapper,
                          const struct rt_arexx_ops *ops,
                          void *ctx,
                          int *quit_requested)
{
    struct RexxMsg *msg;
    int count = 0;

    if (wrapper == NULL || wrapper->port == NULL) return -1;

    while ((msg = (struct RexxMsg *)GetMsg(wrapper->port)) != NULL) {
        struct rt_arexx_result result;
        const char *command = msg->rm_Args[0] != NULL ? (const char *)msg->rm_Args[0] : "";

        rt_arexx_dispatch(command, ops, ctx, &result);
        msg->rm_Result1 = result.rc;
        msg->rm_Result2 = 0;
        if ((msg->rm_Action & RXFF_RESULT) != 0 && result.result[0] != '\0') {
            STRPTR arg = CreateArgstring((STRPTR)result.result,
                                         (ULONG)strlen(result.result));
            if (arg != NULL) msg->rm_Result2 = (LONG)arg;
        }
        if (result.quit && quit_requested != NULL) *quit_requested = 1;
        ReplyMsg((struct Message *)msg);
        ++count;
    }
    return count;
}

#else

struct rt_arexx_port { int unused; };

struct rt_arexx_port *rt_arexx_port_open(void) { return NULL; }
void rt_arexx_port_close(struct rt_arexx_port *port) { (void)port; }
unsigned long rt_arexx_port_signal_mask(const struct rt_arexx_port *port)
{ (void)port; return 0u; }
int rt_arexx_port_pending(const struct rt_arexx_port *port)
{ (void)port; return 0; }
int rt_arexx_port_process(struct rt_arexx_port *port,
                          const struct rt_arexx_ops *ops,
                          void *ctx,
                          int *quit_requested)
{ (void)port; (void)ops; (void)ctx; (void)quit_requested; return 0; }

#endif
