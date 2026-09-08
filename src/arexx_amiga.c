#include "arexx_amiga.h"

#ifdef __AMIGA__

#include <exec/ports.h>
#include <proto/exec.h>
#include <stdlib.h>
#include <string.h>

struct rt_arexx_port {
    struct MsgPort *port;
};

struct rt_arexx_port *rt_arexx_port_open(void)
{
    struct rt_arexx_port *wrapper;
    struct MsgPort *port;

    if (FindPort("REXXTELNET") != NULL) return NULL;

    wrapper = (struct rt_arexx_port *)malloc(sizeof(*wrapper));
    if (wrapper == NULL) return NULL;

    port = CreateMsgPort();
    if (port == NULL) {
        free(wrapper);
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

#else

struct rt_arexx_port { int unused; };

struct rt_arexx_port *rt_arexx_port_open(void) { return NULL; }
void rt_arexx_port_close(struct rt_arexx_port *port) { (void)port; }
unsigned long rt_arexx_port_signal_mask(const struct rt_arexx_port *port)
{ (void)port; return 0u; }
int rt_arexx_port_pending(const struct rt_arexx_port *port)
{ (void)port; return 0; }

#endif
