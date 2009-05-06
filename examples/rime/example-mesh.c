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

#define NF_TIME 10
#define NF_CHAN 1

process_event_t ev_pressed;

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
  printf("NF data received from %d.%d: %.*s (%d)\n",
	 from->u8[0], from->u8[1],
	 packetbuf_datalen(), (char *)packetbuf_dataptr(), packetbuf_datalen());
  printf(" %s",(char *)packetbuf_dataptr());

  return 1;

}

static struct netflood_conn nf;
static const struct netflood_callbacks nf_cb = {nf_recv, NULL, NULL};

void
kbi7_isr(void) 
{
	process_post(&example_mesh_process,ev_pressed,"button");
	clear_kbi_evnt(7);
	return;
}

PROCESS_THREAD(example_mesh_process, ev, data)
{
  PROCESS_EXITHANDLER(netflood_close(&nf);)
  PROCESS_BEGIN();

  netflood_open(&nf,NF_TIME,NF_CHAN,&nf_cb);

  ev_pressed = process_alloc_event();

  while(1) {
    rimeaddr_t addr;
    static struct etimer et;
    static uint32_t tic = 0;
    uint32_t i;

    etimer_set(&et, CLOCK_SECOND); 

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et) ||
			     (ev == ev_pressed));

    if(ev == ev_pressed) 
    {
	    printf("netflood event\n\r");
	    printf("data: %s\n\r",(char *)data);
	    packetbuf_copyfrom(data,(uint16_t)strlen(data));
	    netflood_send(&nf,tic);
    }
    
    if(etimer_expired(&et)) 
    {
	    printf("tic %x\n\r",tic++);
    }

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
