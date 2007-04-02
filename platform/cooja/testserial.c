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
 * $Id: testserial.c,v 1.2 2007/04/02 16:31:28 fros4943 Exp $
 */


#include "contiki.h"
#include "sys/loader.h"

#include <stdio.h>

#include "lib/list.h"
#include "lib/random.h"

#include "net/uip.h"

#include "lib/sensors.h"
#include "sys/log.h"
#include "dev/serial.h"
#include "dev/rs232.h"


PROCESS(serial_test_process, "Serial test process");

AUTOSTART_PROCESSES(&serial_test_process);

PROCESS_THREAD(serial_test_process, ev, data)
{
  static struct etimer mytimer;

  PROCESS_BEGIN();

  etimer_set(&mytimer, CLOCK_SECOND);

  /* Starts the serial process among other */
  serial_init();

  log_message("Starting serial test process\n", "");

  while(1) {
    PROCESS_WAIT_EVENT();
	
    if (etimer_expired(&mytimer)) {
      log_message("Sending serial data now\n", "");
      etimer_restart(&mytimer);
      rs232_print("GNU's not Unix\n");
    }

    if(ev == serial_event_message) {
      log_message("Message received: ", data);
    }
  }

  PROCESS_END();
}
