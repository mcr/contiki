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
 * @(#)$Id: clock.c,v 1.14 2008/07/07 23:38:09 adamdunkels Exp $
 */


#include <io.h>
#include <signal.h>

#include "contiki-conf.h"

#include "sys/energest.h"
#include "sys/clock.h"
#include "sys/etimer.h"

/* 38400 cycles @ 2.4576MHz with divisor 8 ==> 1/8 s */
/* #define INTERVAL (307200ULL / CLOCK_SECOND) */
#define INTERVAL (4096ULL / CLOCK_SECOND)

#define MAX_TICKS (~((clock_time_t)0) / 2)

static volatile unsigned short seconds;

static volatile clock_time_t count = 0;
/* last_tar is used for calculating clock_fine, last_ccr might be better? */
static unsigned short last_tar = 0;
/*---------------------------------------------------------------------------*/
interrupt(TIMERA1_VECTOR) timera1 (void) {
  ENERGEST_ON(ENERGEST_TYPE_IRQ);
  if(TAIV == 2) {
    /* Make sure interrupt time is future */
    do {
      TACCR1 += INTERVAL;
      ++count;

      /* Make sure the CLOCK_CONF_SECOND is a power of two, to ensure
	 that the modulo operation below becomes a logical and and not
	 an expensive divide. Algorithm from Wikipedia:
	 http://en.wikipedia.org/wiki/Power_of_two */
#if (CLOCK_CONF_SECOND & (CLOCK_CONF_SECOND - 1)) != 0
#error CLOCK_CONF_SECOND must be a power of two (i.e., 1, 2, 4, 8, 16, 32, 64, ...).
#error Change CLOCK_CONF_SECOND in contiki-conf.h.
#endif
      if(count % CLOCK_CONF_SECOND == 0) {
	++seconds;
      }
    } while((TACCR1 - TAR) > INTERVAL);

    last_tar = TAR;

    if(etimer_pending() &&
       (etimer_next_expiration_time() - count - 1) > MAX_TICKS) {
      etimer_request_poll();
      LPM4_EXIT;
    }
  }
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  return count;
}
/*---------------------------------------------------------------------------*/
void
clock_set(clock_time_t clock, clock_time_t fclock)
{
  TAR = fclock;
  TACCR1 = fclock + INTERVAL;
  count = clock;
}
/*---------------------------------------------------------------------------*/
int
clock_fine_max(void)
{
  return INTERVAL;
}
/*---------------------------------------------------------------------------*/
unsigned short
clock_fine(void)
{
  unsigned short t;
  /* Assign last_tar to local varible that can not be changed by interrupt */
  t = last_tar;
  /* perform calc based on t, TAR will not be changed during interrupt */
  return (unsigned short) (TAR - t);
}
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  dint();

  /* Select SMCLK (2.4576MHz), clear TAR */
  /* TACTL = TASSEL1 | TACLR | ID_3; */
  
  /* Select ACLK 32768Hz clock, divide by 8 */
  TACTL = TASSEL0 | TACLR | ID_3;

  /* Initialize ccr1 to create the X ms interval. */
  /* CCR1 interrupt enabled, interrupt occurs when timer equals CCR1. */
  TACCTL1 = CCIE;

  /* Interrupt after X ms. */
  TACCR1 = INTERVAL;

  /* Start Timer_A in continuous mode. */
  TACTL |= MC1;

  count = 0;

  /* Enable interrupts. */
  eint();

}
/*---------------------------------------------------------------------------*/
/**
 * Delay the CPU for a multiple of 2.83 us.
 */
void
clock_delay(unsigned int i)
{
  asm("add #-1, r15");
  asm("jnz $-2");
  /*
   * This means that delay(i) will delay the CPU for CONST + 3x
   * cycles. On a 2.4756 CPU, this means that each i adds 1.22us of
   * delay.
   *
   * do {
   *   --i;
   * } while(i > 0);
   */
}
/*---------------------------------------------------------------------------*/
/**
 * Wait for a multiple of 10 ms.
 *
 */
void
clock_wait(int i)
{
  clock_time_t start;

  start = clock_time();
  while(clock_time() - start < (clock_time_t)i);
}
/*---------------------------------------------------------------------------*/
void
clock_set_seconds(unsigned long sec)
{

}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
  static unsigned long offset;
  static unsigned long last_time;
  unsigned long new_time;

  new_time = seconds + offset;

  /*  printf("Seconds %d new_time %lu offset %lu\n", seconds, new_time, offset);*/
  
  if(new_time < last_time) {
    offset += 0x10000L;
    new_time += 0x10000L;
  }

  last_time = new_time;

  return new_time;
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
clock_counter(void)
{
  return TAR;
}
/*---------------------------------------------------------------------------*/
