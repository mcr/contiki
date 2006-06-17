/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 * This file is part of the Contiki operating system.
 *
 * @(#)$Id$
 */

#include <avr/io.h>
#include <avr/signal.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "contiki.h"
#include "dev/slip.h"
#include "dev/serial.h"
#include "dev/rs232.h"

static volatile unsigned char txwait;
/*static unsigned char slipmode;*/

static int (* input_handler)(unsigned char) = NULL;

/*---------------------------------------------------------------------------*/
SIGNAL(SIG_UART0_TRANS)
{
  txwait = 0;
}
/*---------------------------------------------------------------------------*/
SIGNAL(SIG_UART0_RECV)
{
  unsigned char c;

  c = UDR0;

  if(input_handler != NULL) {
    input_handler(c);
  }
  
  /*  if(slipmode) {
    slip_input_byte(c);
  } else {
    serial_input_byte(c);
    }*/
}
/*---------------------------------------------------------------------------*/
void
rs232_init(void)
{
  /* Enable transmit. */
  UCSR0B = _BV(RXCIE) | _BV(TXCIE) | _BV(RXEN) | _BV(TXEN);
  /* Set baud rate (23 =~ 38400) */
  UBRR0H = 0;
  UBRR0L = 23;

  /*  slipmode = 0;*/
  txwait = 0;

  input_handler = NULL;
}
/*---------------------------------------------------------------------------*/
/*void
rs232_init_slip(void)
{
  rs232_init();
  slipmode  = 1;
}*/
/*---------------------------------------------------------------------------*/
void
rs232_print_p(prog_char *buf)
{
  while(pgm_read_byte(buf)) {
    rs232_send(pgm_read_byte(buf));
    ++buf;
  }
}
/*---------------------------------------------------------------------------*/
void
rs232_print(char *buf)
{
  while(*buf) {
    rs232_send(*buf++);
  }
}
/*---------------------------------------------------------------------------*/
void
rs232_send(unsigned char c)
{
  txwait = 1;
  UDR0 = c;
  while(txwait);
}
/*---------------------------------------------------------------------------*/
void
rs232_set_input(int (*f)(unsigned char))
{
  input_handler = f;
}
/*---------------------------------------------------------------------------*/
void
slip_arch_writeb(unsigned char c)
{
  rs232_send(c);
}
/*---------------------------------------------------------------------------*/
