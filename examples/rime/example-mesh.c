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
 * $Id: example-mesh.c,v 1.4 2009/03/12 21:58:21 adamdunkels Exp $
 */

/**
 * \file
 *         A brief description of what this file is.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime.h"
#include "net/rime/mesh.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>
#include <string.h>

#include "kbi.h"
#include "crm.h"
#include "utils.h"
#include "timer.h"
#include "gpio.h"

#define NF_TIME 10
#define NF_CHAN 128

static volatile int beeping=0;
process_event_t ev_pressed;
process_event_t ev_recv;

static struct mesh_conn mesh;
/*---------------------------------------------------------------------------*/
PROCESS(example_mesh_process, "Mesh example");
AUTOSTART_PROCESSES(&example_mesh_process);
/*---------------------------------------------------------------------------*/
static void
sent(struct mesh_conn *c)
{
  printf("packet sent\n");
}
static void
timedout(struct mesh_conn *c)
{
  printf("packet timedout\n");
}

static int
nf_recv(struct netflood_conn *nf, rimeaddr_t *from,
		      rimeaddr_t *originator, uint8_t seqno, uint8_t hops)
{
  printf("NF data received from %d.%d: (%d)\n",
	 from->u8[0], from->u8[1], packetbuf_datalen());

  if(strncmp(packetbuf_dataptr(),"button",packetbuf_datalen())==0) {
	  process_post(&example_mesh_process,ev_recv,"button");
	  printf("posting recv event\n\r");
  }

  return 1;
}

static struct netflood_conn nf;
static const struct netflood_callbacks nf_cb = {nf_recv, NULL, NULL};

static volatile uint8_t led10 = 0;

#define beeper_on()  do {            \
(reg16(TMR2_CSCTRL) =0x0040);        \
} while(0)
#define beeper_off()  do {            \
(reg16(TMR2_CSCTRL) =0x0000);        \
clear_bit(reg32(GPIO_DATA0),10);     \
led10 = 0;                           \
} while(0)


void
kbi7_isr(void) 
{
	process_post(&example_mesh_process,ev_pressed,"button");
	process_post(&example_mesh_process,ev_recv,"button");
	clear_kbi_evnt(7);
	return;
}

/* I can't get this to drive the output directly for some reason */
/* should probably just toggle this in an interrupt for now... */

void
beep_init(void)
{
	/* timer setup */
	/* CTRL */
#define COUNT_MODE 1      /* use rising edge of primary source */
#define PRIME_SRC  0xd    /* Perip. clock with 32 prescale (for 24Mhz = 187500Hz)*/
#define SEC_SRC    0      /* don't need this */
#define ONCE       0      /* keep counting */
#define LEN        1      /* count until compare then reload with value in LOAD */
#define DIR        0      /* count up */
#define CO_INIT    0      /* other counters cannot force a re-initialization of this counter */
#define OUT_MODE   3      /* toggle OFLAG */

//	reg16(TMR_ENBL) = 0;                     /* tmrs reset to enabled */
//	reg16(TMR2_SCTRL) = 1;                   /* output enable */
	reg16(TMR2_SCTRL) = 0;                   
//	reg16(TMR2_CSCTRL) =0x0040;              /* enable compare 1 interrupt */
	reg16(TMR2_CSCTRL) =0x0000;              /* enable compare 1 interrupt */
	reg16(TMR2_LOAD) = 0;                    /* reload to zero */
	reg16(TMR2_COMP_UP) = 187;             /* trigger a reload at the end */
	reg16(TMR2_CMPLD1) = 187;              /* compare 1 triggered reload level, 10HZ maybe? */
	reg16(TMR2_CNTR) = 0;                    /* reset count register */
	reg16(TMR2_CTRL) = (COUNT_MODE<<13) | (PRIME_SRC<<9) | (SEC_SRC<<7) | (ONCE<<6) | (LEN<<5) | (DIR<<4) | (CO_INIT<<3) | (OUT_MODE);
//	reg16(TMR_ENBL) = 0xf;                   /* enable all the timers --- why not? */


}

void tmr2_isr(void) {

	if(bit_is_set(reg16(TMR(2,CSCTRL)),TCF1) &&
	   bit_is_set(reg16(TMR(2,CSCTRL)),TCF1EN)) {
		if(led10 == 0) {
			set_bit(reg32(GPIO_DATA0),10);
			led10 = 1;
		} else {
			clear_bit(reg32(GPIO_DATA0),10);
			led10 = 0;
		}
		/* clear the compare flags */
		clear_bit(reg16(TMR(2,SCTRL)),TCF);                
		clear_bit(reg16(TMR(2,CSCTRL)),TCF1);                
		clear_bit(reg16(TMR(2,CSCTRL)),TCF2);                
		return;
	} else {
		/* this timer didn't create an interrupt condition */
		return;
	}
}


PROCESS_THREAD(example_mesh_process, ev, data)
{
  PROCESS_EXITHANDLER(netflood_close(&nf);)
  PROCESS_BEGIN();

  netflood_open(&nf,NF_TIME,NF_CHAN,&nf_cb);

  ev_pressed = process_alloc_event();
  ev_recv = process_alloc_event();

  beep_init();

  while(1) {
    rimeaddr_t addr;
    static struct etimer et;
    static uint32_t tic = 0;
    uint32_t i;

    etimer_set(&et, CLOCK_SECOND); 

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et) ||
			     (ev == ev_pressed) ||
			     (ev == ev_recv)
	    );

    if(ev == ev_pressed) 
    {
	    printf("netflood event\n\r");
	    printf("data: %s\n\r",(char *)data);
	    packetbuf_copyfrom(data,(uint16_t)strlen(data));
	    netflood_send(&nf,tic);
    }

    if(ev == ev_recv) 
    {
	    printf("recv event\n\r");
	    printf("data: %s\n\r",(char *)data);

	    if(strcmp(data,"button")==0) 
	    {
		    beeping = 1;
		    beeper_on();
	    }
    }

    if(etimer_expired(&et)) 
    {
	    beeper_off();
	    printf("tic %x\n\r",tic++);
    }

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
