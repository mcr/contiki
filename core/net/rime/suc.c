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
 * $Id: suc.c,v 1.8 2007/03/29 23:18:22 adamdunkels Exp $
 */

/**
 * \file
 *         A brief description of what this file is.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "net/rime/suc.h"
#include "net/rime.h"
#include <string.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
static void
recv_from_uc(struct uc_conn *uc, rimeaddr_t *from)
{
  register struct suc_conn *c = (struct suc_conn *)uc;
  PRINTF("%d: suc: recv_from_uc from %d %p\n",
	 rimeaddr_node_addr.u16, from->u16, c);
  if(c->u->recv != NULL) {
    c->u->recv(c, from);
  }
}
/*---------------------------------------------------------------------------*/
static const struct uc_callbacks suc = {recv_from_uc};
/*---------------------------------------------------------------------------*/
void
suc_open(struct suc_conn *c, u16_t channel,
	  const struct suc_callbacks *u)
{
  uc_open(&c->c, channel, &suc);
  c->u = u;
}
/*---------------------------------------------------------------------------*/
void
suc_close(struct suc_conn *c)
{
  uc_close(&c->c);
  ctimer_stop(&c->t);
  if(c->buf != NULL) {
    queuebuf_free(c->buf);
  }
}
/*---------------------------------------------------------------------------*/
static void
send(void *ptr)
{
  struct suc_conn *c = ptr;

  PRINTF("%d: suc: resend to %d\n",
	 rimeaddr_node_addr.u16, c->receiver.u16);
  queuebuf_to_rimebuf(c->buf);
  uc_send(&c->c, &c->receiver);
  suc_set_timer(c, CLOCK_SECOND);
  if(c->u->sent != NULL) {
    c->u->sent(c);
  }
}
/*---------------------------------------------------------------------------*/
void
suc_set_timer(struct suc_conn *c, clock_time_t t)
{
  ctimer_set(&c->t, t, send, c);
}
/*---------------------------------------------------------------------------*/
int
suc_send_stubborn(struct suc_conn *c, rimeaddr_t *receiver)
{
  if(c->buf != NULL) {
    queuebuf_free(c->buf);
  }
  c->buf = queuebuf_new_from_rimebuf();
  if(c->buf == NULL) {
    return 0;
  }
  rimeaddr_copy(&c->receiver, receiver);
  ctimer_set(&c->t, CLOCK_SECOND, send, c);

  PRINTF("%d: suc_send_stubborn to %d\n", rimeaddr_node_addr.u16, c->receiver.u16);
  uc_send(&c->c, &c->receiver);
  if(c->u->sent != NULL) {
    c->u->sent(c);
  }
  
  return 1;
  
}
/*---------------------------------------------------------------------------*/
int
suc_send(struct suc_conn *c, rimeaddr_t *receiver)
{
  return uc_send(&c->c, receiver);
}
/*---------------------------------------------------------------------------*/
void
suc_cancel(struct suc_conn *c)
{
  ctimer_stop(&c->t);
}
/*---------------------------------------------------------------------------*/
