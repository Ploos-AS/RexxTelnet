#ifndef REXXTELNET_AREXX_AMIGA_H
#define REXXTELNET_AREXX_AMIGA_H

struct rt_arexx_port;

struct rt_arexx_port *rt_arexx_port_open(void);
void rt_arexx_port_close(struct rt_arexx_port *port);
unsigned long rt_arexx_port_signal_mask(const struct rt_arexx_port *port);
int rt_arexx_port_pending(const struct rt_arexx_port *port);

#endif
