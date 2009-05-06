/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 * $Id: hello-world.c,v 1.1 2006/10/02 21:46:46 adamdunkels Exp $
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"

#include "utils.h"
#include "gpio.h"
#include "uart1.h"

#define DELAY8  100000
#define DELAY9  200000
#define DELAY10 400000


#include <stdio.h> /* For printf() */


static struct etimer blink8_timer, blink9_timer, blink10_timer;

PROCESS(blink8_process, "blink8 process");
PROCESS(blink9_process, "blink9 process");
PROCESS(blink10_process, "blink10 process");

PROCESS_NAME(blink8_process);
PROCESS_NAME(blink9_process);
PROCESS_NAME(blink10_process);

AUTOSTART_PROCESSES(&blink8_process,&blink9_process,&blink10_process);

PROCESS_THREAD(blink8_process, ev, data)
{

  PROCESS_BEGIN();

  etimer_set(&blink8_timer, CLOCK_SECOND);

  set_bit(reg32(GPIO_PAD_DIR0),8);
  
  volatile uint32_t i;

  static volatile uint8_t led8 = 0;
  
  while(1) {
	  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
	  
	  if(data == &blink8_timer) {
		  if(led8 == 0) {
			  set_bit(reg32(GPIO_DATA0),8);
			  led8 = 1;
//			  printf("Red\n\r");
		  } else {
			  clear_bit(reg32(GPIO_DATA0),8);
			  led8 = 0;
		  }
		  etimer_reset(&blink8_timer);
	  }
  };
  
  PROCESS_END();
}


PROCESS_THREAD(blink9_process, ev, data)
{

  PROCESS_BEGIN();

  etimer_set(&blink9_timer, CLOCK_SECOND/2);

  set_bit(reg32(GPIO_PAD_DIR0),9);
  
  volatile uint32_t i;

  static volatile uint8_t led9 = 0;
  
  while(1) {
	  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
	  
	  if(data == &blink9_timer) {
		  if(led9 == 0) {
			  set_bit(reg32(GPIO_DATA0),9);
			  led9 = 1;
//			  printf("Green\n\r");
		  } else {
			  clear_bit(reg32(GPIO_DATA0),9);
			  led9 = 0;
		  }
		  etimer_reset(&blink9_timer);
	  }
  };
  
  PROCESS_END();
}



PROCESS_THREAD(blink10_process, ev, data)
{

  PROCESS_BEGIN();

  etimer_set(&blink10_timer, CLOCK_SECOND/4);

  set_bit(reg32(GPIO_PAD_DIR0),10);
  
  volatile uint32_t i;

  static volatile uint8_t led10 = 0;
  
  while(1) {
	  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
	  
	  if(data == &blink10_timer) {
		  if(led10 == 0) {
			  set_bit(reg32(GPIO_DATA0),10);
			  led10 = 1;
//			  printf("Blue\n\r");
		  } else {
			  clear_bit(reg32(GPIO_DATA0),10);
			  led10 = 0;
		  }
		  etimer_reset(&blink10_timer);
	  }
  };
  
  PROCESS_END();
}

