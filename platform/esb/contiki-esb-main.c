/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 * @(#)$Id$
 */

#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "sys/procinit.h"
#include "sys/autostart.h"
#include "contiki-esb.h"

#include "cfs/cfs-eeprom.h"

SENSORS(&button_sensor, &sound_sensor, &vib_sensor,
	&pir_sensor, &radio_sensor, &battery_sensor, &ctsrts_sensor,
	&temperature_sensor);

PROCINIT(&sensors_process, &ir_process, &etimer_process,
	 &tcpip_process, &uip_fw_process, &cfs_eeprom_process);

#define ENABLE_AUTOSTART 0

PROCESS(contiki_esb_main_init_process, "Contiki ESB init process");

PROCESS_THREAD(contiki_esb_main_init_process, ev, data)
{
  PROCESS_BEGIN();

  procinit_init();

  PROCESS_PAUSE();

  init_net();

  PROCESS_PAUSE();

  init_apps();

  PROCESS_PAUSE();

  autostart_start((struct process **) autostart_processes);

  beep_spinup();
  leds_on(LEDS_ALL);
  clock_delay(100);
  leds_off(LEDS_ALL);


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
int
main(void)
{
  msp430_cpu_init();

  init_lowlevel();

  clock_init();

  process_init();

  random_init(0);

  uip_init();
  uip_fw_init();

  process_start(&contiki_esb_main_init_process, NULL);

  /*  watchdog_init();*/
  
  /*  beep();*/

  while(1) {
    /*    watchdog_restart();*/
    while(process_run() > 0);
    LPM_SLEEP();
  }


  return 0;
}
/*---------------------------------------------------------------------------*/
char *arg_alloc(char size) {return NULL;}
void  arg_init(void) {}
void  arg_free(char *arg) {}
/*---------------------------------------------------------------------------*/

int
putchar(int c)
{
  rs232_send(c);
  return c;
}

void
uip_log(char *m)
{
  printf("uIP log: '%s'\n", m);
}
