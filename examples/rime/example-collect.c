/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 *         Example of how the collect primitive works.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "lib/random.h"
#include "net/rime.h"
#include "net/rime/collect.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"

#include <stdio.h>

#include "mc1322x.h"

#define TIC 4
#define ON_BATTERY 0
#define CHIRP_INTERVAL  10  /* seconds */
#define CHIRPS          5 
#define SLEEP_TIME 300 /* seconds */

static struct collect_conn tc;
process_event_t ev_pressed;

/*---------------------------------------------------------------------------*/
PROCESS(example_collect_process, "Test collect process");
AUTOSTART_PROCESSES(&example_collect_process);
/*---------------------------------------------------------------------------*/

#define led_blue_on() do { set_bit(*GPIO_DATA0,10); set_bit(*GPIO_DATA0,25); } while (0)
#define led_blue_off() do { clear_bit(*GPIO_DATA0,10); clear_bit(*GPIO_DATA0,25); } while(0)

void safe_sleep(void) {
	/* set kbi wake up polarity to the opposite of the current reading */
	if(bit_is_set(*GPIO_DATA0,29)) {
		printf("pol neg\n\r");
		kbi_pol_neg(7);
	} else {
		printf("pol pos\n\r");
		kbi_pol_pos(7);
	}
	*CRM_WU_TIMEOUT = cal_rtc_secs * SLEEP_TIME;
	sleep((SLEEP_RETAIN_MCU|SLEEP_RAM_64K),SLEEP_MODE_HIBERNATE);
	enable_irq(TMR);
	enable_irq(CRM);
	uart_init(INC, MOD, SAMP);
	maca_on();
	printf("awake\n\r");
	if(bit_is_set(*CRM_STATUS,7)) {
		printf("woke up from button\n\r");
		process_post(&example_collect_process, ev_pressed, "GPIO29");
	}
	clear_ext_wu_evt(7);
}

void report_state(void) {
	packetbuf_clear();
	if(bit_is_set(*GPIO_DATA0,29)) {
		packetbuf_copyfrom("GPIO29-1",9);
	} else {
		packetbuf_copyfrom("GPIO29-0",9);
	}
	collect_send(&tc, 4);
}

void
kbi7_isr(void) 
{
	led_blue_on();
	printf("button7\n\r");
	clear_kbi_evnt(7);
	process_post(&example_collect_process, ev_pressed, "GPIO29");
	led_blue_off();
	return;
}

static void
recv(const rimeaddr_t *originator, uint8_t seqno, uint8_t hops)
{
  printf("Sink got message from %d.%d, seqno %d, hops %d: len %d '%s'\n",
	 originator->u8[0], originator->u8[1],
	 seqno, hops,
	 packetbuf_datalen(),
	 (char *)packetbuf_dataptr());

}
/*---------------------------------------------------------------------------*/
static const struct collect_callbacks callbacks = { recv };
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_collect_process, ev, data)
{
  PROCESS_BEGIN();

  ev_pressed = process_alloc_event();
  collect_open(&tc, 128, &callbacks);

#if ON_BATTERY
  enable_timer_wu();
#endif
  
  if(rimeaddr_node_addr.u8[0] == 1) {
	  printf("I am sink\n");
	  collect_set_sink(&tc, 1);
  }
  
  while(1) {
	  static struct etimer et;
	  static uint16_t chirps;
	  
#if ON_BATTERY
	  /* platform is set to wake up on kbi7 */
	  /* safe_sleep sets it to sleep for SLEEP_TIME */
	  chirps = 0;
	  
	  for(chirps=0; chirps<CHIRPS; chirps++) {
		  
		  etimer_set(&et, CLOCK_SECOND*CHIRP_INTERVAL);
		  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et) ||
					   (ev == ev_pressed)
			  );
		  
		  if(etimer_expired(&et) || 
		     (ev == ev_pressed)) {
			  printf("chirp %d\n\r",chirps);
			  while(tc.forwarding) {
				  PROCESS_PAUSE();
			  }
			  report_state();
		  }
	  }
	  
	  printf("going to sleep\n");
	  safe_sleep();
  	  
#else
	  etimer_set(&et, CLOCK_SECOND * TIC + random_rand() % (CLOCK_SECOND * TIC));
	  PROCESS_WAIT_EVENT();
	  
	  if(etimer_expired(&et)) {
		  while(tc.forwarding) {
			  PROCESS_PAUSE();
		  }
		  report_state();
	  }

//	  if(ev == ev_pressed) {
//		  report_state();
//	  }


#endif
	  
	  
  }
  
  PROCESS_END();
  
  }

/*---------------------------------------------------------------------------*/
