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
 * $Id: tree.h,v 1.5 2007/03/25 12:06:39 adamdunkels Exp $
 */

/**
 * \file
 *         A brief description of what this file is.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef __TREE_H__
#define __TREE_H__

#include "net/rime/uibc.h"
#include "net/rime/ruc.h"

struct tree_callbacks {
  void (* recv)(rimeaddr_t *originator, u8_t seqno,
		u8_t hops, u8_t retransmissions);
};

struct tree_conn {
  struct uibc_conn uibc_conn;
  struct ruc_conn ruc_conn;
  u8_t forwarding;
  u8_t hops_from_sink;
  u8_t seqno;
  const struct tree_callbacks *cb;
};

void tree_open(struct tree_conn *c, u16_t channels,
	       const struct tree_callbacks *callbacks);
void tree_close(struct tree_conn *c);

void tree_send(struct tree_conn *c);

void tree_set_sink(struct tree_conn *c, int should_be_sink);

int tree_depth(struct tree_conn *c);

#define TREE_MAX_DEPTH 63

#endif /* __TREE_H__ */
