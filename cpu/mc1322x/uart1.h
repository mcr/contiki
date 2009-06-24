#ifndef UART1_H
#define UART1_H

#define BASE_UART1      0x80005000
#define UART1_CON       0x80005000
#define UART1_STAT      0x80005004
#define UART1_DATA      0x80005008
#define UR1CON          0x8000500c
#define UT1CON          0x80005010
#define UART1_CTS       0x80005014
#define UART1_BR        0x80005018

int uart1_putchar(int c);

#define uart1_can_get() (reg32(UR1CON) > 0)

#endif
