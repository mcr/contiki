/**
 * \addtogroup rime
 * @{
 */

/**
 * \defgroup rimemesh Mesh routing
 * @{
 *
 * The abc module sends packets using multi-hop routing to a specified
 * receiver somewhere in the network.
 *
 */

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
 * $Id: mesh.h,v 1.7 2007/03/29 23:18:48 adamdunkels Exp $
 */

/**
 * \file
 *         Header file for the Rime mesh routing protocol
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef __MESH_H__
#define __MESH_H__

#include "net/rime.h"
#include "net/rime/mh.h"
#include "net/rime/route-discovery.h"

struct mesh_conn;

/**
 * \brief     Mesh callbacks
 */
struct mesh_callbacks {
  /** Called when a packet is received. */
  void (* recv)(struct mesh_conn *c, rimeaddr_t *from);
  /** Called when a packet, sent with mesh_send(), is actually transmitted. */
  void (* sent)(struct mesh_conn *c);
  /** Called when a packet, sent with mesh_send(), times out and is dropped. */
  void (* timedout)(struct mesh_conn *c);
};

struct mesh_conn {
  struct mh_conn mh;
  struct route_discovery_conn route_discovery_conn;
  struct queuebuf *queued_data;
  rimeaddr_t queued_data_dest;
  const struct mesh_callbacks *cb;
};

/**
 * \brief      Open a mesh connection
 * \param c    A pointer to a struct mesh_conn
 * \param channels The channels on which the connection will operate; mesh uses 3 channels
 * \param callbacks Pointer to callback structure
 *
 *             This function sets up a mesh connection on the
 *             specified channel. The caller must have allocated the
 *             memory for the struct mesh_conn, usually by declaring it
 *             as a static variable.
 *
 *             The struct mesh_callbacks pointer must point to a structure
 *             containing function pointers to functions that will be called
 *             when a packet arrives on the channel.
 *
 */
void mesh_open(struct mesh_conn *c, u16_t channels,
	       const struct mesh_callbacks *callbacks);

/**
 * \brief      Close an mesh connection
 * \param c    A pointer to a struct mesh_conn
 *
 *             This function closes an mesh connection that has
 *             previously been opened with mesh_open().
 *
 *             This function typically is called as an exit handler.
 *
 */
void mesh_close(struct mesh_conn *c);

/**
 * \brief      Send a mesh packet
 * \param c    The mesh connection on which the packet should be sent
 * \param dest The address of the final destination of the packet
 * \retval     Non-zero if the packet could be queued for sending, zero otherwise
 *
 *             This function sends a mesh packet. The packet must be
 *             present in the rimebuf before this function is called.
 *
 *             The parameter c must point to an abc connection that
 *             must have previously been set up with mesh_open().
 *
 */
int mesh_send(struct mesh_conn *c, rimeaddr_t *dest);

#endif /* __MESH_H__ */
/** @} */
/** @} */
