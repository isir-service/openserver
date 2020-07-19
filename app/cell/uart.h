#ifndef __UART_H__
#define __UART_H__

int uart_init(char *dev_name);
void uart_read(int s, short what, void *arg);

#endif
