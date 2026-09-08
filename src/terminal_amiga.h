#ifndef REXXTELNET_TERMINAL_AMIGA_H
#define REXXTELNET_TERMINAL_AMIGA_H

int rt_terminal_amiga_open(void);
int rt_terminal_amiga_has_input(void);
long rt_terminal_amiga_write(const unsigned char *data, unsigned long len);
long rt_terminal_amiga_read(unsigned char *data, unsigned long len);
void rt_terminal_amiga_close(void);

#endif
