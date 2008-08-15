/*
 * Copyright (c) 2008, Swedish Institute of Computer Science.
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
 * $Id: shell-sky.c,v 1.11 2008/08/15 19:07:04 adamdunkels Exp $
 */

/**
 * \file
 *         Tmote Sky-specific Contiki shell commands
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "shell-sky.h"

#include "dev/watchdog.h"

#include "net/rime.h"
#include "dev/cc2420.h"
#include "dev/leds.h"
#include "dev/light.h"
#include "dev/sht11.h"

#include "net/rime/timesynch.h"

#include "node-id.h"

#include <stdio.h>
#include <string.h>

struct power_msg {
  uint16_t len;
  uint32_t cpu;
  uint32_t lpm;
  uint32_t transmit;
  uint32_t listen;
};

#define WITH_POWERGRAPH 0

/*---------------------------------------------------------------------------*/
PROCESS(shell_nodeid_process, "nodeid");
SHELL_COMMAND(nodeid_command,
	      "nodeid",
	      "nodeid: set node ID",
	      &shell_nodeid_process);
PROCESS(shell_sense_process, "sense");
SHELL_COMMAND(sense_command,
	      "sense",
	      "sense: print out sensor data",
	      &shell_sense_process);
PROCESS(shell_senseconv_process, "senseconv");
SHELL_COMMAND(senseconv_command,
	      "senseconv",
	      "senseconv: convert 'sense' data to human readable format",
	      &shell_senseconv_process);
PROCESS(shell_txpower_process, "txpower");
SHELL_COMMAND(txpower_command,
	      "txpower",
	      "txpower <power>: change CC2420 transmission power (0 - 31)",
	      &shell_txpower_process);
PROCESS(shell_rfchannel_process, "rfchannel");
SHELL_COMMAND(rfchannel_command,
	      "rfchannel",
	      "rfchannel <channel>: change CC2420 radio channel (11 - 26)",
	      &shell_rfchannel_process);
PROCESS(shell_power_process, "power");
SHELL_COMMAND(power_command,
	      "power",
	      "power: print power profile",
	      &shell_power_process);
PROCESS(shell_energy_process, "energy");
SHELL_COMMAND(energy_command,
	      "energy",
	      "energy: print energy profile",
	      &shell_energy_process);
PROCESS(shell_powerconv_process, "powerconv");
SHELL_COMMAND(powerconv_command,
	      "powerconv",
	      "powerconv: convert power profile to human readable output",
	      &shell_powerconv_process);
#if WITH_POWERGRAPH
PROCESS(shell_powergraph_process, "powergraph");
SHELL_COMMAND(powergraph_command,
	      "powergraph",
	      "powergraph: convert power profile to a 'graphical' repressentation",
	      &shell_powergraph_process);
#endif /* WITH_POWERGRAPH */
/*---------------------------------------------------------------------------*/
#define MAX(a, b) ((a) > (b)? (a): (b))
#define MIN(a, b) ((a) < (b)? (a): (b))
struct spectrum {
  int channel[16];
};
#define NUM_SAMPLES 4
static struct spectrum rssi_samples[NUM_SAMPLES];
static int
do_rssi(void)
{
  static int sample;
  int channel;
  
  rime_mac->off(0);

  cc2420_on();
  for(channel = 11; channel <= 26; ++channel) {
    cc2420_set_channel(channel);
    rssi_samples[sample].channel[channel - 11] = cc2420_rssi() + 53;
  }
  
  rime_mac->on();
  
  sample = (sample + 1) % NUM_SAMPLES;

  {
    int channel, tot;
    tot = 0;
    for(channel = 0; channel < 16; ++channel) {
      int max = -256;
      int i;
      for(i = 0; i < NUM_SAMPLES; ++i) {
	max = MAX(max, rssi_samples[i].channel[channel]);
      }
      tot += max / 20;
    }
    return tot;
  }
}
/*---------------------------------------------------------------------------*/
struct sense_msg {
  uint16_t len;
  uint16_t clock;
  uint16_t timesynch_time;
  uint16_t light1;
  uint16_t light2;
  uint16_t temp;
  uint16_t humidity;
  uint16_t rssi;
};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_sense_process, ev, data)
{
  struct sense_msg msg;
  PROCESS_BEGIN();

  msg.len = 7;
  msg.clock = clock_time();
  msg.timesynch_time = timesynch_time();
  msg.light1 = sensors_light1();
  msg.light2 = sensors_light2();
  msg.temp = sht11_temp();
  msg.humidity = sht11_humidity();
  msg.rssi = do_rssi();

  shell_output(&sense_command, &msg, sizeof(msg), "", 0);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_senseconv_process, ev, data)
{
  struct shell_input *input;
  struct sense_msg *msg;
  PROCESS_BEGIN();
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == shell_event_input);
    input = data;

    if(input->len1 + input->len2 == 0) {
      PROCESS_EXIT();
    }
    msg = (struct sense_msg *)input->data1;

    if(msg != NULL) {
      char buf[40];
      snprintf(buf, sizeof(buf),
	       "%d", 10 * msg->light1 / 7);
      shell_output_str(&senseconv_command, "Light 1 ", buf);
      snprintf(buf, sizeof(buf),
	       "%d", 46 * msg->light2 / 10);
      shell_output_str(&senseconv_command, "Light 2 ", buf);
      snprintf(buf, sizeof(buf),
	       "%d.%d", (msg->temp / 10 - 396) / 10,
	       (msg->temp / 10 - 396) % 10);
      shell_output_str(&senseconv_command, "Temperature ", buf);
      snprintf(buf, sizeof(buf),
	       "%d", (int)(-4L + 405L * msg->humidity / 10000L));
      shell_output_str(&senseconv_command, "Relative humidity ", buf);
      snprintf(buf, sizeof(buf),
	       "%d", msg->rssi);
      shell_output_str(&senseconv_command, "RSSI ", buf);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_txpower_process, ev, data)
{
  struct {
    uint16_t len;
    uint16_t txpower;
  } msg;
  const char *newptr;
  PROCESS_BEGIN();

  msg.txpower = shell_strtolong(data, &newptr);
  
  /* If no transmission power was given on the command line, we print
     out the current txpower. */
  
  if(newptr == data) {
    msg.txpower = cc2420_get_txpower();
  } else {
    cc2420_set_txpower(msg.txpower);
  }

  msg.len = 1;

  shell_output(&txpower_command, &msg, sizeof(msg), "", 0);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_rfchannel_process, ev, data)
{
  struct {
    uint16_t len;
    uint16_t channel;
  } msg;
  const char *newptr;
  PROCESS_BEGIN();

  msg.channel = shell_strtolong(data, &newptr);
  
  /* If no channel was given on the command line, we print out the
     current channel. */
  if(newptr == data) {
    msg.channel = cc2420_get_channel();
  } else {
    cc2420_set_channel(msg.channel);
  }

  msg.len = 1;

  shell_output(&txpower_command, &msg, sizeof(msg), "", 0);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_power_process, ev, data)
{
  static unsigned long last_cpu, last_lpm, last_transmit, last_listen;
  struct power_msg msg;

  PROCESS_BEGIN();

  energest_flush();
  
  msg.len = 8;
  msg.cpu = energest_type_time(ENERGEST_TYPE_CPU) - last_cpu;
  msg.lpm = energest_type_time(ENERGEST_TYPE_LPM) - last_lpm;
  msg.transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT) - last_transmit;
  msg.listen = energest_type_time(ENERGEST_TYPE_LISTEN) - last_listen;

  last_cpu = energest_type_time(ENERGEST_TYPE_CPU);
  last_lpm = energest_type_time(ENERGEST_TYPE_LPM);
  last_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  last_listen = energest_type_time(ENERGEST_TYPE_LISTEN);

  shell_output(&power_command, &msg, sizeof(msg), "", 0);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_energy_process, ev, data)
{
  struct power_msg msg;

  PROCESS_BEGIN();

  energest_flush();
  
  msg.len = 8;
  msg.cpu = energest_type_time(ENERGEST_TYPE_CPU);
  msg.lpm = energest_type_time(ENERGEST_TYPE_LPM);
  msg.transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  msg.listen = energest_type_time(ENERGEST_TYPE_LISTEN);

  shell_output(&energy_command, &msg, sizeof(msg), "", 0);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#define DEC2FIX(h,d) ((h * 64L) + (unsigned long)((d * 64L) / 1000L))
static void
printpower(struct power_msg *msg)
{
  char buf[50];
  unsigned long avg_power;
  unsigned long time;

  time = msg->cpu + msg->lpm;
  
  avg_power = (3L *
	       (msg->cpu       * DEC2FIX(1L,800L) +
		msg->lpm       * DEC2FIX(0L,545L) +
		msg->transmit  * DEC2FIX(17L,700L) +
		msg->listen    * DEC2FIX(20L,0))) / ((64L * time) / 1000);
  snprintf(buf, sizeof(buf), "CPU %d%% LPM %d%% tx %d%% rx %d%% tot %lu uW",
	   (int)((100L * (unsigned long)msg->cpu) / time),
	   (int)((100L * (unsigned long)msg->lpm) / time),
	   (int)((100L * (unsigned long)msg->transmit) / time),
	   (int)((100L * (unsigned long)msg->listen) / time),
	   avg_power);
  shell_output_str(&powerconv_command, buf, "");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_powerconv_process, ev, data)
{
  struct power_msg *msg;
  struct shell_input *input;
  int len;

  PROCESS_BEGIN();
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == shell_event_input);
    input = data;
    
    if(input->len1 + input->len2 == 0) {
      PROCESS_EXIT();
    }
    len = input->len1;
    for(msg = (struct power_msg *)input->data1;
	len > 0;
	msg++, len -= sizeof(struct power_msg)) {
      printpower(msg);
    }
    len = input->len2;
    for(msg = (struct power_msg *)input->data2;
	len > 0;
	msg++, len -= sizeof(struct power_msg)) {
      printpower(msg);
    }

  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#if WITH_POWERGRAPH
#define MAX_POWERGRAPH 34
static void
printpowergraph(struct power_msg *msg)
{
  int i;
  unsigned long avg_power;
  unsigned long time;
  char buf[MAX_POWERGRAPH];

  time = msg->cpu + msg->lpm;
  
  avg_power = (3L *
	       (msg->cpu       * DEC2FIX(1L,800L) +
		msg->lpm       * DEC2FIX(0L,545L) +
		msg->transmit  * DEC2FIX(17L,700L) +
		msg->listen    * DEC2FIX(20L,0))) / ((64L * time) / 1000);
  memset(buf, 0, MAX_POWERGRAPH);
  for(i = 0; avg_power > 0 && i < MAX_POWERGRAPH; ++i) {
    buf[i] = '*';
    avg_power -= MIN(2000, avg_power);
  }
  shell_output_str(&powergraph_command, buf, "");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_powergraph_process, ev, data)
{
  struct power_msg *msg;
  struct shell_input *input;
  int len;

  PROCESS_BEGIN();
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == shell_event_input);
    input = data;
    
    if(input->len1 + input->len2 == 0) {
      PROCESS_EXIT();
    }
    len = input->len1;
    for(msg = (struct power_msg *)input->data1;
	len > 0;
	msg++, len -= sizeof(struct power_msg)) {
      printpowergraph(msg);
    }
    len = input->len2;
    for(msg = (struct power_msg *)input->data2;
	len > 0;
	msg++, len -= sizeof(struct power_msg)) {
      printpowergraph(msg);
    }

  }
  
  PROCESS_END();
}
#endif /* WITH_POWERGRAPH */
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_nodeid_process, ev, data)
{
  uint16_t nodeid;
  char buf[20];
  const char *newptr;
  PROCESS_BEGIN();

  nodeid = shell_strtolong(data, &newptr);
  
  /* If no node ID was given on the command line, we print out the
     current channel. Else we burn the new node ID. */
  if(newptr == data) {
    nodeid = node_id;
  } else {
    nodeid = shell_strtolong(data, &newptr);
    watchdog_stop();
    leds_on(LEDS_RED);
    node_id_burn(nodeid);
    leds_on(LEDS_BLUE);
    node_id_restore();
    leds_off(LEDS_RED + LEDS_BLUE);
    watchdog_start();
  }

  snprintf(buf, sizeof(buf), "%d", nodeid);
  shell_output_str(&nodeid_command, "Node ID: ", buf);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
shell_sky_init(void)
{
  shell_register_command(&power_command);
  shell_register_command(&powerconv_command);
  shell_register_command(&energy_command);
  shell_register_command(&txpower_command);
  shell_register_command(&rfchannel_command);
  shell_register_command(&sense_command);
  shell_register_command(&senseconv_command);
  shell_register_command(&nodeid_command);

#if WITH_POWERGRAPH
  shell_register_command(&powergraph_command);
#endif /* WITH_POWERGRAPH */
}
/*---------------------------------------------------------------------------*/
