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
 * $Id: radio-arch.c,v 1.15 2007/04/23 08:46:35 fros4943 Exp $
 */

#include "dev/radio-arch.h"
#include "dev/radio.h"

#include "lib/simEnvChange.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "net/uip.h"
#include "net/uip-fw.h"
#include "net/rime.h"
#include "sys/etimer.h"
#include "sys/cooja_mt.h"

#include "sys/log.h"

#define MAX_RETRIES 100
#define SS_INTERFERENCE -70

const struct simInterface radio_interface;

// COOJA variables
char simTransmitting;
char simReceiving;

char simInDataBuffer[UIP_BUFSIZE];
int simInSize;
char simOutDataBuffer[UIP_BUFSIZE];
int simOutSize;

char simRadioHWOn = 1;
int simSignalStrength = -200;
char simPower = 100;
int simRadioChannel = 1;

int inSendFunction = 0;

enum {
  UIP,
  RIME,
};

/*-----------------------------------------------------------------------------------*/
void
radio_set_channel(int channel)
{
  simRadioChannel = channel;
}
/*-----------------------------------------------------------------------------------*/
int
radio_sstrength(void)
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
    return;
  }
  
  // Don't fall asleep while receiving (in main file)
  if (simReceiving) {
    simDontFallAsleep = 1;
    return;
  }
  
  // If no incoming radio data, do nothing
  if (simInSize == 0) {
    return;
  }
  
  // Check size of received packet
  if (simInSize > UIP_BUFSIZE) {
    // Drop packet by not delivering
    return;
  }
  
  // ** Good place to add explicit manchester/gcr-encoding
  if(simInDataBuffer[0] == UIP) {
    // Hand over new packet to uIP
    uip_len = simInSize - 1;
    memcpy(&uip_buf[UIP_LLH_LEN], &simInDataBuffer[1], simInSize - 1);
    
    if(simNoYield) {
      simDoTcpipInput = 1;
    } else {
      tcpip_input();
    }
  } else if(simInDataBuffer[0] == RIME) {
    /* If we are not configured to use uIP, we use Rime instead. */
    rimebuf_copyfrom(&simInDataBuffer[1], simInSize - 1);
    /*    log_message("rime input", "");*/
    rime_input();
  }
  
  simInSize = 0;
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
static u8_t
simDoLLSend(unsigned char *buf, int len, int uip_or_rime)
{
  /* If radio already actively transmitting, drop packet*/
  if(inSendFunction) {
    return UIP_FW_DROPPED;
  }

  inSendFunction = 1;

  /* If radio is turned off, do nothing */
  if(!simRadioHWOn) {
    inSendFunction = 0;
    return UIP_FW_DROPPED;
  }

  /* Drop packet if data size too large */
  if(len > UIP_BUFSIZE) {
    inSendFunction = 0;
    return UIP_FW_TOOLARGE;
  }

  /* Drop packet if no data length */
  if(len <= 0) {
    inSendFunction = 0;
    return UIP_FW_ZEROLEN;
  }

  /* ** Good place to add explicit manchester/gcr-decoding */

  /* Copy packet data to temporary storage */
  simOutDataBuffer[0] = uip_or_rime;
  memcpy(&simOutDataBuffer[1], buf, len);
  simOutSize = len + 1;

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
    return UIP_FW_DROPPED;
  }

  // - Initiate transmission -
  simTransmitting = 1;

  // Busy-wait while transmitting
  while(simTransmitting && !simNoYield) {
    cooja_mt_yield();
  }

  inSendFunction = 0;
  return UIP_FW_OK;
}
/*-----------------------------------------------------------------------------------*/
void
rime_driver_send(void)
{
  /*  log_message("rime driver send", "");*/
  simDoLLSend(rimebuf_hdrptr(), rimebuf_totlen(), RIME);
}
/*-----------------------------------------------------------------------------------*/
u8_t
simDoSend()
{
  /* If radio is turned off, reset uip_len */
  if(!simRadioHWOn) {
    uip_len = 0;
    return UIP_FW_DROPPED;
  }
  if(uip_len > UIP_BUFSIZE) {
    uip_len = 0;
    return UIP_FW_DROPPED;
  }
  if(uip_len <= 0) {
    return UIP_FW_ZEROLEN;
  }

  return simDoLLSend(&uip_buf[UIP_LLH_LEN], uip_len, UIP);
}
/*-----------------------------------------------------------------------------------*/
/**
 * \brief      Turn radio on.
 *
 *             This function turns the radio hardware on.
 */
void
radio_on(void)
{
  simRadioHWOn = 1;
}
/*-----------------------------------------------------------------------------------*/
/**
 * \brief      Turn radio off.
 *
 *             This function turns the radio hardware off.
 */
void
radio_off(void)
{
  simRadioHWOn = 0;
}
/*-----------------------------------------------------------------------------------*/
SIM_INTERFACE(radio_interface,
	      doInterfaceActionsBeforeTick,
	      doInterfaceActionsAfterTick);
