/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: ethernode.c,v 1.8 2007/05/22 21:09:19 adamdunkels Exp $
 */
/**
 * \file
 * uIP virtual network interface using UDP tunnels.
 * \author Adam Dunkels
 *
 */

#include "net/ethernode.h"
#include "net/uip_arch.h"
#include "net/uip-fw.h"
#include "ether.h"

#include "dev/radio.h"

#include "node.h"

#include "lib/random.h"

#include <stdio.h>
#include <unistd.h>

#define BUF ((uip_tcpip_hdr *)&uip_buf[HDR_LEN])

#define PRINTF(...)
/*#define PRINTF(x) printf x; fflush(NULL)*/

struct {
  u8_t id;
  int timer;
  u8_t seqno;
} state;

#define TYPE_DATA      1
#define TYPE_ACK       2

struct hdr {
  u8_t type;
  u8_t src;
  u8_t dest;
  u8_t seqno;
};

static int ethernode_on(void) {return 0;}
static int ethernode_safe_off(void) {return 0;}

#include "net/ethernode.h"

const struct radio_driver ethernode_driver =
  {
    ethernode_send_buf,
    ethernode_read,
    ethernode_set_receiver,
    ethernode_on,
    ethernode_safe_off,
  };


#define HDR_LEN UIP_LLH_LEN

#define ID_BROADCAST 0x80

PROCESS(ethernode_process, "Ethernode");
/*-------------------------------------------------------------------------------*/
static u8_t
do_send(u8_t type, u8_t dest, struct hdr *hdr, int len)
{
  
  hdr->type = type;
  hdr->src = state.id;

  hdr->dest = dest;

  hdr->seqno = state.seqno;

  ++state.seqno;

  PRINTF("ether_send len %d\n", len);
  return ether_send((char *)hdr, len);
  
}
/*-------------------------------------------------------------------------------*/
/**
 * Initialize the virtual UDP tunnel network interface.
 *
 * \param sid The ID number of this node.
 *
 */
/*-------------------------------------------------------------------------------*/
void
ethernode_init(int port)
{
  ether_client_init(port);

  do {
    state.id = random_rand() & 0x7f;
  } while(state.id == ID_BROADCAST);

  state.seqno = 0;
}
/*-------------------------------------------------------------------------------*/
/**
 * Poll the network device to see if a packet has arrived.
 *
 * \return The length of the incoming packet, or zero if no packet was
 * found.
 */
/*-------------------------------------------------------------------------------*/
int
ethernode_poll(void)
{
  return ether_client_poll();
}
/*-------------------------------------------------------------------------------*/
u16_t
ethernode_read(u8_t *buf, u16_t bufsize)
{
  int len;
  u8_t tmpbuf[2048];
  struct hdr *hdr = (struct hdr *)tmpbuf;
  
  len = ether_client_read(tmpbuf, sizeof(tmpbuf));
  if(len == 0) {
    return 0;
  }
  
  /*  printf("ethernode_poll: received data packet with len %d type %d\n", len, hdr->type);*/

  switch(hdr->type) {
  case TYPE_DATA:
    if(hdr->dest == state.id ||
       hdr->dest == ID_BROADCAST) {
      memcpy(buf, tmpbuf + HDR_LEN, bufsize);
      return len - HDR_LEN;
    }
    break;
  case TYPE_ACK:
    printf("<%d>: Received ack packet from %d\n", state.id, hdr->src);
    break;
  default:
    printf("<%d>: Received unknown packet type %d from %d\n", state.id, hdr->type, hdr->src);
    break;
  }
  
  return 0;
    
}
/*-------------------------------------------------------------------------------*/
/**
 * Send a packet from the uip_buf buffer over the UDP tunnel.
 *
 *
 * \retval UIP_FW_TOOLARGE A transmission of packet that was too large was attempted.
 *
 * \retval UIP_FW_DROPPED The packet is known to be dropped.
 *
 * \retval UIP_FW_OK The packet was transmitted.
 */
/*-------------------------------------------------------------------------------*/
u8_t
ethernode_send(void)
{
  int len;
  static char tmpbuf[2048];
  struct hdr *hdr = (struct hdr *)tmpbuf;
  u8_t dest;
  
  if(uip_len > sizeof(tmpbuf)) {
    PRINTF(("Ethernode_send: too large uip_len %d\n", uip_len));
    return UIP_FW_TOOLARGE;
  }

  memcpy(&tmpbuf[HDR_LEN], &uip_buf[HDR_LEN], uip_len);
  len = uip_len + HDR_LEN;

  dest = ID_BROADCAST;
  
  usleep(1000 * (random_rand() % 1000));

  do_send(TYPE_DATA, dest, hdr, len);

  return UIP_FW_OK;
}
/*-------------------------------------------------------------------------------*/
int
ethernode_send_buf(const u8_t *buf, u16_t len)
{
  char tmpbuf[2048];
  struct hdr *hdr = (struct hdr *)tmpbuf;
  u8_t dest;

  memcpy(&tmpbuf[HDR_LEN], buf, len);
  len = len + HDR_LEN;

  dest = ID_BROADCAST;
  do_send(TYPE_DATA, dest, hdr, len);
  return len;
}
/*-------------------------------------------------------------------------------*/
static void (* receiver_callback)(const struct radio_driver *);
/*-------------------------------------------------------------------------------*/
void
ethernode_set_receiver(void (* recv)(const struct radio_driver *))
{
  process_start(&ethernode_process, NULL);
  receiver_callback = recv;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ethernode_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    process_poll(&ethernode_process);
    PROCESS_WAIT_EVENT();

    if(ethernode_poll()) {
      if(receiver_callback) {
	receiver_callback(&ethernode_driver);
      }
    }
  }
  PROCESS_END();
}
/*-------------------------------------------------------------------------------*/
