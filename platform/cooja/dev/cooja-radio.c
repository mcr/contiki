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
 * $Id: cooja-radio.c,v 1.2 2007/05/18 15:20:21 fros4943 Exp $
 */

#include <string.h>
#include "contiki.h"

#include "dev/radio.h"
#include "dev/cooja-radio.h"
#include "lib/simEnvChange.h"
#include "sys/cooja_mt.h"

#define MAX_RETRIES 100
#define SS_INTERFERENCE -70

const struct simInterface radio_interface;

// COOJA variables
char simTransmitting;
char simReceiving;

char simInDataBuffer[COOJA_RADIO_BUFSIZE];
int simInSize;
char simInPolled;
char simOutDataBuffer[COOJA_RADIO_BUFSIZE];
int simOutSize;

char simRadioHWOn = 1;
int simSignalStrength = -200;
int simLastSignalStrength = -200;
char simPower = 100;
int simRadioChannel = 1;
int inSendFunction = 0;

static void (* receiver_callback)(const struct radio_driver *);

const struct radio_driver cooja_driver =
  {
    radio_send,
    radio_read,
    radio_set_receiver,
    radio_on,
    radio_off,
  };

/*-----------------------------------------------------------------------------------*/
void
radio_set_receiver(void (* recv)(const struct radio_driver *))
{
  receiver_callback = recv;
}
/*-----------------------------------------------------------------------------------*/
int
radio_on(void)
{
  simRadioHWOn = 1;
  return 1;
}
/*-----------------------------------------------------------------------------------*/
int
radio_off(void)
{
  simRadioHWOn = 0;
  return 1;
}
/*---------------------------------------------------------------------------*/
void
radio_set_channel(int channel)
{
  simRadioChannel = channel;
}
/*-----------------------------------------------------------------------------------*/
int
radio_sstrength(void)
{
  return simLastSignalStrength;
}
/*-----------------------------------------------------------------------------------*/
int
radio_current_sstrength(void)
{
  return simSignalStrength;
}
/*-----------------------------------------------------------------------------------*/
void
radio_set_txpower(unsigned char power)
{
  /* 1 - 100: Number indicating output power */
  simPower = power;
}
/*-----------------------------------------------------------------------------------*/
static void
doInterfaceActionsBeforeTick(void)
{
  // If radio is turned off, do nothing
  if (!simRadioHWOn) {
    simInSize = 0;
    simInPolled = 0;
    return;
  }
  
  // Don't fall asleep while receiving (in main file)
  if (simReceiving) {
    simLastSignalStrength = simSignalStrength;
    simDontFallAsleep = 1;
    return;
  }
  
  // If no incoming radio data, do nothing
  if (simInSize == 0) {
    simInPolled = 0;
    return;
  }
  
  // Check size of received packet
  if (simInSize > COOJA_RADIO_BUFSIZE) {
    // Drop packet by not delivering
    return;
  }
  
  // ** Good place to add explicit manchester/gcr-encoding
  
  if(receiver_callback != NULL && !simInPolled) {
    receiver_callback(&cooja_driver);
    simInPolled = 1;
  }
}
/*---------------------------------------------------------------------------*/
u16_t
radio_read(u8_t *buf, u16_t bufsize)
{
  int tmpInSize = simInSize;
  if(simInSize > 0) {
    
    memcpy(buf, simInDataBuffer, simInSize);
    simInSize = 0;
    return tmpInSize;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
static void
doInterfaceActionsAfterTick(void)
{
  // Make sure we are awake during radio activity
  if (simReceiving || simTransmitting) {
    simDontFallAsleep = 1;
    return;
  }
}
/*-----------------------------------------------------------------------------------*/
int
radio_send(const u8_t *payload, u16_t payload_len)
{
  /* If radio already actively transmitting, drop packet*/
  if(inSendFunction) {
    return COOJA_RADIO_DROPPED;
  }
  
  inSendFunction = 1;
  
  /* If radio is turned off, do nothing */
  if(!simRadioHWOn) {
    inSendFunction = 0;
    return COOJA_RADIO_DROPPED;
  }
  
  /* Drop packet if data size too large */
  if(payload_len > COOJA_RADIO_BUFSIZE) {
    inSendFunction = 0;
    return COOJA_RADIO_TOOLARGE;
  }
  
  /* Drop packet if no data length */
  if(payload_len <= 0) {
    inSendFunction = 0;
    return COOJA_RADIO_ZEROLEN;
  }
  
  /* ** Good place to add explicit manchester/gcr-decoding */
  
  /* Copy packet data to temporary storage */
  memcpy(simOutDataBuffer, payload, payload_len);
  simOutSize = payload_len;
  
  /* Busy-wait until both radio HW and ether is ready */
  {
    int retries = 0;
    while(retries < MAX_RETRIES && !simNoYield &&
	  (simSignalStrength > SS_INTERFERENCE || simReceiving)) {
      retries++;
      cooja_mt_yield();
      if(!(simSignalStrength > SS_INTERFERENCE || simReceiving)) {
	/* Wait one extra tick before transmission starts */
	cooja_mt_yield();
      }
    }
  }
  
  if(simSignalStrength > SS_INTERFERENCE || simReceiving) {
    inSendFunction = 0;
    return COOJA_RADIO_DROPPED;
  }
  
  // - Initiate transmission -
  simTransmitting = 1;
  
  // Busy-wait while transmitting
  while(simTransmitting && !simNoYield) {
    cooja_mt_yield();
  }
  
  inSendFunction = 0;
  return COOJA_RADIO_OK;
}
/*-----------------------------------------------------------------------------------*/
SIM_INTERFACE(radio_interface,
	      doInterfaceActionsBeforeTick,
	      doInterfaceActionsAfterTick);
