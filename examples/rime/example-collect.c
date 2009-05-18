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
 * $Id: example-collect.c,v 1.8 2009/03/12 21:58:21 adamdunkels Exp $
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
#include "net/rime/neighbor.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"

#include <stdio.h>

#include "gpio.h"
#include "utils.h"
#include "crm.h"
#include "isr.h"

#define TIC 4
#define ON_BATTERY 0
#define CHIRP_INTERVAL  3  /* seconds */
#define CHIRPS          3 
#define SLEEP_TIME 300 /* seconds */

static struct collect_conn tc;
process_event_t ev_pressed;

/*---------------------------------------------------------------------------*/
PROCESS(example_collect_process, "Test collect process");
AUTOSTART_PROCESSES(&example_collect_process);
/*---------------------------------------------------------------------------*/

#define REF_OSC 24000000UL          /* reference osc. frequency */
#define NOMINAL_RING_OSC_SEC 2000 /* nominal ring osc. frequency */
static uint32_t hib_wake_secs;      /* calibrated hibernate wake seconds */

void safe_sleep(void) {
	/* set kbi wake up polarity to the opposite of the current reading */
	if(bit_is_set(reg32(GPIO_DATA0),29)) {
		printf("pol neg\n\r");
		kbi_pol_neg(7);
	} else {
		printf("pol pos\n\r");
		kbi_pol_pos(7);
	}
	reg32(CRM_WU_TIMEOUT) = hib_wake_secs * SLEEP_TIME;
	sleep((SLEEP_RETAIN_MCU|SLEEP_RAM_64K),SLEEP_MODE_HIBERNATE);
	enable_irq(TMR);
	enable_irq(CRM);
	uart1_init();
	maca_on();
	printf("awake\n\r");
	if(bit_is_set(reg32(CRM_STATUS),7)) {
		printf("woke up from button\n\r");
		process_post(&example_collect_process, ev_pressed, "GPIO29");
	}
	clear_ext_wu_evt(7);
}

void report_state(void) {
	packetbuf_clear();
	if(bit_is_set(reg32(GPIO_DATA0),29)) {
		packetbuf_copyfrom("GPIO29-1",9);
	} else {
		packetbuf_copyfrom("GPIO29-0",9);
	}
	collect_send(&tc, 4);
}

void
kbi7_isr(void) 
{
	set_bit(reg32(GPIO_DATA0),10);
	printf("button7\n\r");
	clear_kbi_evnt(7);
	process_post(&example_collect_process, ev_pressed, "GPIO29");
	clear_bit(reg32(GPIO_DATA0),10);
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
uint32_t cal_factor;
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

  printf("performing ring osc cal\n\r");
  printf("sys_cntl: 0x%0x\n\r",reg32(CRM_SYS_CNTL)); 
  reg32(CRM_CAL_CNTL) = (1<<16) | (2000); 
  while((reg32(CRM_STATUS) & (1<<9)) == 0);
  printf("ring osc cal complete\n\r");
  printf("cal_count: 0x%0x\n\r",reg32(CRM_CAL_COUNT)); 
  cal_factor = REF_OSC*100/reg32(CRM_CAL_COUNT);
  hib_wake_secs = (NOMINAL_RING_OSC_SEC * cal_factor)/100;
  printf("cal factor: %d \n\nr", cal_factor);
  printf("hib_wake_secs: %d \n\nr", hib_wake_secs);

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
