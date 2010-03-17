/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 *         A generic module for management of neighbor information.
 *
 * \author Nicolas Tsiftes <nvt@sics.se>
 */

#include "net/neighbor-info.h"
#include "net/neighbor-attr.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

#ifndef ETX_CONF_LIMIT
#define ETX_LIMIT		10
#else
#define ETX_LIMIT		ETX_CONF_LIMIT
#endif /* !ETX_CONF_LIMIT */

#define ETX_SCALE		100
#define ETX_ALPHA		60
#define ETX_FIRST_GUESS		1
#define ETX_COLLISION_PENALTY	5
#define ETX_NO_ACK_PENALTY	(ETX_LIMIT + 1)
/*---------------------------------------------------------------------------*/
NEIGHBOR_ATTRIBUTE(uint8_t, etx, NULL);

static neighbor_info_subscriber_t subscriber_callback;
/*---------------------------------------------------------------------------*/
static void
update_etx(const rimeaddr_t *dest, int packet_etx)
{
  uint8_t *etxp;
  uint8_t recorded_etx, new_etx;

  etxp = (uint8_t *)neighbor_attr_get_data(&etx, dest);
  if(etxp == NULL || *etxp == 0) {
    recorded_etx = ETX_FIRST_GUESS;
  } else {
    recorded_etx = *etxp;
  }

  new_etx = (ETX_SCALE / 2 + ETX_ALPHA * recorded_etx +
		 (ETX_SCALE - ETX_ALPHA) * packet_etx) / ETX_SCALE;
  PRINTF("neighbor-info: ETX changed from %d to %d (packet ETX = %d)\n", 
	recorded_etx, new_etx, packet_etx);

  if(new_etx > ETX_LIMIT) {
    neighbor_attr_remove_neighbor(dest);
    subscriber_callback(dest, 0, new_etx);
  } else {
    neighbor_attr_set_data(&etx, dest, &new_etx);
    if(new_etx != recorded_etx && subscriber_callback != NULL) {
      subscriber_callback(dest, 1, new_etx);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
add_neighbor(const rimeaddr_t *addr)
{
  switch(neighbor_attr_add_neighbor(addr)) {
  case -1:
    PRINTF("neighbor-info: failed to add a node.\n");
    break;
  case 0:
    PRINTF("neighbor-info: The neighbor is already known\n");
    break;
  default:
    if(subscriber_callback != NULL) {
      subscriber_callback(addr, 1, ETX_FIRST_GUESS);
    }
    break;
  }
}
/*---------------------------------------------------------------------------*/
void
neighbor_info_packet_sent(int status, int numtx)
{
  const rimeaddr_t *dest;
  uint8_t packet_etx;

  dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(rimeaddr_cmp(dest, &rimeaddr_null)) {
    return;
  }

  PRINTF("neighbor-info: packet sent to %d.%d, status=%d, numtx=%d\n", 
	dest->u8[sizeof(*dest) - 2], dest->u8[sizeof(*dest) - 1],
	status, numtx);

  packet_etx = numtx;
  switch(status) {
  case MAC_TX_OK:
    add_neighbor(dest);
    break;
  case MAC_TX_COLLISION:
    packet_etx += ETX_COLLISION_PENALTY; 
    break;
  case MAC_TX_NOACK:
    packet_etx += ETX_NO_ACK_PENALTY;
    break;
  default:
    break;
  }

  update_etx(dest, packet_etx);
}
/*---------------------------------------------------------------------------*/
void
neighbor_info_packet_received(void)
{
  const rimeaddr_t *src;

  src = packetbuf_addr(PACKETBUF_ADDR_SENDER);
  if(rimeaddr_cmp(src, &rimeaddr_null)) {
    return;
  }

  PRINTF("neighbor-info: packet received from %d.%d\n",
	src->u8[sizeof(*src) - 2], src->u8[sizeof(*src) - 1]);

  add_neighbor(src);
}
/*---------------------------------------------------------------------------*/
int
neighbor_info_subscribe(neighbor_info_subscriber_t s)
{
  if(subscriber_callback == NULL) {
    subscriber_callback = s;
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
