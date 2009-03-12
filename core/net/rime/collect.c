/* XXX: send explicit congestion notification if already forwarding
   packet + add queue for keeping packets to forward. */

/**
 * \addtogroup rimecollect
 * @{
 */

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
 * $Id: collect.c,v 1.23 2009/03/12 21:58:21 adamdunkels Exp $
 */

/**
 * \file
 *         Tree-based hop-by-hop reliable data collection
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"

#include "net/rime.h"
#include "net/rime/neighbor.h"
#include "net/rime/collect.h"

#include "dev/radio-sensor.h"

#if CONTIKI_TARGET_NETSIM
#include "ether.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stddef.h>

static const struct packetbuf_attrlist attributes[] =
  {
    COLLECT_ATTRIBUTES
    PACKETBUF_ATTR_LAST
  };

#define NUM_RECENT_PACKETS 2

struct recent_packet {
  rimeaddr_t originator;
  uint8_t seqno;
};

static struct recent_packet recent_packets[NUM_RECENT_PACKETS];
static uint8_t recent_packet_ptr;

#define SINK 0
#define RTMETRIC_MAX COLLECT_MAX_DEPTH

#define MAX_HOPLIM 10

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
static void
update_rtmetric(struct collect_conn *tc)
{
  struct neighbor *n;

  /* We should only update the rtmetric if we are not the sink. */
  if(tc->rtmetric != SINK) {

    /* Find the neighbor with the lowest rtmetric. */
    n = neighbor_best();

    /* If n is NULL, we have no best neighbor. */
    if(n == NULL) {

      /* If we have don't have any neighbors, we set our rtmetric to
	 the maximum value to indicate that we do not have a route. */
      
      if(tc->rtmetric != RTMETRIC_MAX) {
	PRINTF("%d.%d: didn't find a best neighbor, setting rtmetric to max\n",
	       rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
      }
      tc->rtmetric = RTMETRIC_MAX;
			announcement_set_value(&tc->announcement, tc->rtmetric);
    } else {

      /* We set our rtmetric to the rtmetric of our best neighbor plus
	 the expected transmissions to reach that neighbor. */
      if(n->rtmetric + neighbor_etx(n) != tc->rtmetric) {
	tc->rtmetric = n->rtmetric + neighbor_etx(n);
	/*	neighbor_discovery_start(&tc->neighbor_discovery_conn, tc->rtmetric);*/
	announcement_set_value(&tc->announcement, tc->rtmetric);
	PRINTF("%d.%d: new rtmetric %d\n",
	       rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	       tc->rtmetric);
      }
    }
  }

  /*  DEBUG_PRINTF("%d: new rtmetric %d\n", node_id, rtmetric);*/
#if CONTIKI_TARGET_NETSIM
  {
    char buf[8];
    if(tc->rtmetric == RTMETRIC_MAX) {
      strcpy(buf, " ");
    } else {
      sprintf(buf, "%.1f", (float)tc->rtmetric / NEIGHBOR_ETX_SCALE);
    }
    ether_set_text(buf);
  }
#endif
}
/*---------------------------------------------------------------------------*/
static void
node_packet_received(struct runicast_conn *c, rimeaddr_t *from, uint8_t seqno)
{
  struct collect_conn *tc = (struct collect_conn *)
    ((char *)c - offsetof(struct collect_conn, runicast_conn));
  struct neighbor *n;
  int i;

  /* To protect against forwarding duplicate packets, we keep a list
     of recently forwarded packet seqnos. If the seqno of the current
     packet exists in the list, we drop the packet. */

  for(i = 0; i < NUM_RECENT_PACKETS; i++) {
    if(recent_packets[i].seqno == packetbuf_attr(PACKETBUF_ATTR_EPACKET_ID) &&
	  rimeaddr_cmp(&recent_packets[i].originator,
		       packetbuf_addr(PACKETBUF_ADDR_ESENDER))) {
      PRINTF("%d.%d: dropping duplicate packet with seqno %d\n",
	     packetbuf_attr(PACKETBUF_ATTR_EPACKET_ID));
      /* Drop the packet. */
      return;
    }
  }
  recent_packets[recent_packet_ptr].seqno = packetbuf_attr(PACKETBUF_ATTR_EPACKET_ID);
  rimeaddr_copy(&recent_packets[recent_packet_ptr].originator,
		packetbuf_addr(PACKETBUF_ADDR_ESENDER));
  recent_packet_ptr = (recent_packet_ptr + 1) % NUM_RECENT_PACKETS;
  
  if(tc->rtmetric == SINK) {

    /* If we are the sink, we call the receive function. */
    
    PRINTF("%d.%d: sink received packet from %d.%d via %d.%d\n",
	   rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	   packetbuf_addr(PACKETBUF_ADDR_ESENDER)->u8[0],
	   packetbuf_addr(PACKETBUF_ADDR_ESENDER)->u8[1],
	   from->u8[0], from->u8[1]);

    if(tc->cb->recv != NULL) {
      tc->cb->recv(packetbuf_addr(PACKETBUF_ADDR_ESENDER),
		   packetbuf_attr(PACKETBUF_ATTR_EPACKET_ID),
		   packetbuf_attr(PACKETBUF_ATTR_HOPS));
    }
    return;
  } else if(packetbuf_attr(PACKETBUF_ATTR_TTL) > 1 &&
	    tc->rtmetric != RTMETRIC_MAX) {

    /* If we are not the sink, we forward the packet to the best
       neighbor. */
    packetbuf_set_attr(PACKETBUF_ATTR_HOPS, packetbuf_attr(PACKETBUF_ATTR_HOPS) + 1);
    packetbuf_set_attr(PACKETBUF_ATTR_TTL, packetbuf_attr(PACKETBUF_ATTR_TTL) - 1);

        
    PRINTF("%d.%d: packet received from %d.%d via %d.%d, forwarding %d\n",
	   rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	   packetbuf_addr(PACKETBUF_ADDR_ESENDER)->u8[0],
	   packetbuf_addr(PACKETBUF_ADDR_ESENDER)->u8[1],
	   from->u8[0], from->u8[1], tc->forwarding);

    if(!tc->forwarding) {
      n = neighbor_best();
      if(n != NULL && !rimeaddr_cmp(&n->addr, from)) {
#if CONTIKI_TARGET_NETSIM
	ether_set_line(n->addr.u8[0], n->addr.u8[1]);
#endif /* CONTIKI_TARGET_NETSIM */
	tc->forwarding = 1;
	runicast_send(c, &n->addr, packetbuf_attr(PACKETBUF_ATTR_MAX_REXMIT));
      } else {
	PRINTF("%d.%d: did not find any neighbor to forward to\n",
	       rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
      }
      return;
    } else {

      PRINTF("%d.%d: still forwarding another packet\n",
	     rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
      return;
    }
  }
  return;
}
/*---------------------------------------------------------------------------*/
static void
node_packet_sent(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions)
{
  struct collect_conn *tc = (struct collect_conn *)
    ((char *)c - offsetof(struct collect_conn, runicast_conn));

  tc->forwarding = 0;
  neighbor_update_etx(neighbor_find(to), retransmissions);
  update_rtmetric(tc);
}
/*---------------------------------------------------------------------------*/
static void
node_packet_timedout(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions)
{
  struct collect_conn *tc = (struct collect_conn *)
    ((char *)c - offsetof(struct collect_conn, runicast_conn));

  tc->forwarding = 0;
  neighbor_timedout_etx(neighbor_find(to), retransmissions);
  update_rtmetric(tc);
}
/*---------------------------------------------------------------------------*/
/*static void
adv_received(struct neighbor_discovery_conn *c, rimeaddr_t *from, uint16_t rtmetric)
{
  struct collect_conn *tc = (struct collect_conn *)
    ((char *)c - offsetof(struct collect_conn, neighbor_discovery_conn));
  struct neighbor *n;
  
  n = neighbor_find(from);

  if(n == NULL) {
    neighbor_add(from, rtmetric, 1);
  } else {
    neighbor_update(n, rtmetric);
    PRINTF("%d.%d: updating neighbor %d.%d, etx %d\n",
	   rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	   n->addr.u8[0], n->addr.u8[1], rtmetric);
  }

  update_rtmetric(tc);
}*/
static void
received_announcement(struct announcement *a, rimeaddr_t *from,
		      uint16_t id, uint16_t value)
{
  struct collect_conn *tc = (struct collect_conn *)
    ((char *)a - offsetof(struct collect_conn, announcement));
  struct neighbor *n;
  
  n = neighbor_find(from);

  if(n == NULL) {
    neighbor_add(from, value, 1);
  } else {
    neighbor_update(n, value);
    PRINTF("%d.%d: updating neighbor %d.%d, etx %d\n",
	   rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	   n->addr.u8[0], n->addr.u8[1], value);
  }

  update_rtmetric(tc);  
}
/*---------------------------------------------------------------------------*/
static const struct runicast_callbacks runicast_callbacks = {node_packet_received,
						   node_packet_sent,
						   node_packet_timedout};
/*static const struct neighbor_discovery_callbacks neighbor_discovery_callbacks =
  { adv_received, NULL};*/
/*---------------------------------------------------------------------------*/
void
collect_open(struct collect_conn *tc, uint16_t channels,
	     const struct collect_callbacks *cb)
{
  /*  neighbor_discovery_open(&tc->neighbor_discovery_conn, channels,
			  CLOCK_SECOND * 2,
			  CLOCK_SECOND * 10,
			  CLOCK_SECOND * 60,
			  &neighbor_discovery_callbacks);*/
  runicast_open(&tc->runicast_conn, channels + 1, &runicast_callbacks);
  channel_set_attributes(channels + 1, attributes);
  tc->rtmetric = RTMETRIC_MAX;
  tc->cb = cb;
  /*  neighbor_discovery_start(&tc->neighbor_discovery_conn, tc->rtmetric);*/
  announcement_register(&tc->announcement, channels, tc->rtmetric,
			received_announcement);
  announcement_listen(2);
}
/*---------------------------------------------------------------------------*/
void
collect_close(struct collect_conn *tc)
{
  /*  neighbor_discovery_close(&tc->neighbor_discovery_conn);*/
  announcement_remove(&tc->announcement);
  runicast_close(&tc->runicast_conn);
}
/*---------------------------------------------------------------------------*/
void
collect_set_sink(struct collect_conn *tc, int should_be_sink)
{
  if(should_be_sink) {
    tc->rtmetric = SINK;
    /*    neighbor_discovery_start(&tc->neighbor_discovery_conn, tc->rtmetric);*/
    announcement_set_value(&tc->announcement, tc->rtmetric);
  } else {
    tc->rtmetric = RTMETRIC_MAX;
  }
  announcement_set_value(&tc->announcement, tc->rtmetric);
  update_rtmetric(tc);
}
/*---------------------------------------------------------------------------*/
int
collect_send(struct collect_conn *tc, int rexmits)
{
  struct neighbor *n;
  
  packetbuf_set_attr(PACKETBUF_ATTR_EPACKET_ID, tc->seqno++);
  packetbuf_set_addr(PACKETBUF_ADDR_ESENDER, &rimeaddr_node_addr);
  packetbuf_set_attr(PACKETBUF_ATTR_HOPS, 1);
  packetbuf_set_attr(PACKETBUF_ATTR_TTL, MAX_HOPLIM);
  packetbuf_set_attr(PACKETBUF_ATTR_MAX_REXMIT, rexmits);

  if(tc->rtmetric == 0) {
    packetbuf_set_attr(PACKETBUF_ATTR_HOPS, 0);
    if(tc->cb->recv != NULL) {
      tc->cb->recv(packetbuf_addr(PACKETBUF_ADDR_ESENDER),
		   packetbuf_attr(PACKETBUF_ATTR_EPACKET_ID),
		   packetbuf_attr(PACKETBUF_ATTR_HOPS));
    }
    return 1;
  } else {
    n = neighbor_best();
    if(n != NULL) {
#if CONTIKI_TARGET_NETSIM
      ether_set_line(n->addr.u8[0], n->addr.u8[1]);
#endif /* CONTIKI_TARGET_NETSIM */
      PRINTF("%d.%d: sending to %d.%d\n",
	     rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	     n->addr.u8[0], n->addr.u8[1]);
      tc->forwarding = 1;
      return runicast_send(&tc->runicast_conn, &n->addr, rexmits);
    } else {
      /*      printf("Didn't find any neighbor\n");*/
      PRINTF("%d.%d: did not find any neighbor to send to\n",
	     rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
      announcement_listen(1);
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
collect_depth(struct collect_conn *tc)
{
  return tc->rtmetric;
}
/*---------------------------------------------------------------------------*/
/** @} */
