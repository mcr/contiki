/**
 * \addtogroup rimeibc
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
 * $Id$
 */

/**
 * \file
 *         Identified best-effort local area broadcast (ibc)
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki-net.h"
#include <string.h>

static const struct rimebuf_attrlist attributes[] =
  {
    IBC_ATTRIBUTES RIMEBUF_ATTR_LAST
  };

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
static void
recv_from_abc(struct abc_conn *bc)
{
  rimeaddr_t sender;
  struct ibc_conn *c = (struct ibc_conn *)bc;

  rimeaddr_copy(&sender, rimebuf_addr(RIMEBUF_ADDR_SENDER));
  
  PRINTF("%d.%d: ibc: from %d.%d\n",
	 rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
	 sender.u8[0], sender.u8[1]);
  c->u->recv(c, &sender);
}
/*---------------------------------------------------------------------------*/
static const struct abc_callbacks ibc = {recv_from_abc};
/*---------------------------------------------------------------------------*/
void
ibc_open(struct ibc_conn *c, uint16_t channel,
	  const struct ibc_callbacks *u)
{
  abc_open(&c->c, channel, &ibc);
  c->u = u;
  channel_set_attributes(channel, attributes);
}
/*---------------------------------------------------------------------------*/
void
ibc_close(struct ibc_conn *c)
{
  abc_close(&c->c);
}
/*---------------------------------------------------------------------------*/
int
ibc_send(struct ibc_conn *c)
{
  PRINTF("%d.%d: ibc_send\n",
	 rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1]);
  rimebuf_set_addr(RIMEBUF_ADDR_SENDER, &rimeaddr_node_addr);
  return abc_send(&c->c);
}
/*---------------------------------------------------------------------------*/
/** @} */
