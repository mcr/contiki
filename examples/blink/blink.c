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
 * $Id$
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "dev/leds.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static struct etimer red_timer, green_timer, blue_timer;

PROCESS(red_process, "red process");
PROCESS(green_process, "green process");
PROCESS(blue_process, "blue process");

PROCESS_NAME(red_process);
PROCESS_NAME(green_process);
PROCESS_NAME(blue_process);

AUTOSTART_PROCESSES(&red_process, &green_process, &blue_process);

PROCESS_THREAD(red_process, ev, data)
{

	PROCESS_BEGIN();

	leds_init();
	etimer_set(&red_timer, CLOCK_SECOND);

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

		if(data == &red_timer) {
			leds_toggle(LEDS_RED);
			PRINTF("Red\n\r");
			etimer_reset(&red_timer);
		}
	}

	PROCESS_END();
}


PROCESS_THREAD(green_process, ev, data)
{

	PROCESS_BEGIN();

	etimer_set(&green_timer, CLOCK_SECOND/2);

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

		if(data == &green_timer) {
			leds_toggle(LEDS_GREEN);
			PRINTF("Green\n\r");
			etimer_reset(&green_timer);
		}
	};

	PROCESS_END();
}



PROCESS_THREAD(blue_process, ev, data)
{

	PROCESS_BEGIN();

	etimer_set(&blue_timer, CLOCK_SECOND/4);

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

		if(data == &blue_timer) {
			leds_toggle(LEDS_BLUE);
			PRINTF("Blue\n\r");
			etimer_reset(&blue_timer);
		}
	};

	PROCESS_END();
}

