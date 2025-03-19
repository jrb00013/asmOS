#ifndef UART_H
#define UART_H

#include <stdio.h>

// Function prototypes for UART operations
void init_uart(void);
void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char *s);

#endif // UART_H
