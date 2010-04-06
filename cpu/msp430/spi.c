/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * @(#)$Id$
 */

#include <io.h>

#include "contiki-conf.h"

/*
 * On the Tmote sky access to I2C/SPI/UART0 must always be
 * exclusive. Set spi_busy so that interrupt handlers can check if
 * they are allowed to use the bus or not. Only the CC2420 radio needs
 * this in practice.
 * 
 */
unsigned char spi_busy = 0;

/*
 * Initialize SPI bus.
 */
void
spi_init(void)
{
  static unsigned char spi_inited = 0;

  if (spi_inited)
    return;
#ifdef __msp430_headers_usart_h

  /* MSP430x161x have USART */
  /* Initalize ports for communication with SPI units. */

  U0CTL  = CHAR + SYNC + MM + SWRST; /* SW  reset,8-bit transfer, SPI master */
  U0TCTL = CKPH + SSEL1 + STC;	/* Data on Rising Edge, SMCLK, 3-wire. */

  U0BR0  = 0x02;		/* SPICLK set baud. */
  U0BR1  = 0;  /* Dont need baud rate control register 2 - clear it */
  U0MCTL = 0;			/* Dont need modulation control. */

  P3SEL |= BV(SCK) | BV(MOSI) | BV(MISO); /* Select Peripheral functionality */
  P3DIR |= BV(SCK) | BV(MISO);	/* Configure as outputs(SIMO,CLK). */

  ME1   |= USPIE0;	   /* Module enable ME1 --> U0ME? xxx/bg */
  U0CTL &= ~SWRST;		/* Remove RESET */

#elif defined(__MSP430_HEADERS_USCI_H__)
#ifdef UCB1_SPI_MASTER
  /* MSP430x54xx have USCI */

  UCB1CTL1 |= UCSWRST;                      /* **Put state machine in reset** */
  UCB1CTL0 = UCMST+UCSYNC+UCCKPL+UCMSB;    /*  3-pin, 8-bit SPI master */
                                            /*  Clock polarity high, MSB */
  UCB1CTL1 = (UCB1CTL1&0x3f) | UCSSEL_SMCLK;                     /*  SMCLK */
  UCB1BR0 = 0x02;                           /* /2 */
  UCB1BR1 = 0;                              /*  */
  UCB1MCTL = 1;                             /*  No modulation */

  /* Configure as outputs */
  P3REN &= 0x7f;
  P5REN &= ~(0x20);  /* SPI CLK P5.5 */

  /* Select Peripheral functionality */
  P3SEL |= 0x80;  /* SPI MOSI P3.7 */
  P5SEL |= 0x10;  /* SPI MISO P5.4 */
  P5SEL |= 0x20;  /* SPI CLK P5.5 */
  UCB1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
#endif 
#endif 
}
