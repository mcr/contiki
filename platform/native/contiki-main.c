/*
 * Copyright (c) 2002, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki OS
 *
 * $Id: contiki-main.c,v 1.3 2007/05/22 21:33:31 oliverschmidt Exp $
 *
 */

#include <stdio.h>
#include <unistd.h>

#include "contiki.h"

#include "net/uip.h"

#include "dev/button-sensor.h"
#include "dev/pir-sensor.h"
#include "dev/vib-sensor.h"

PROCINIT(&etimer_process, &tcpip_process);

SENSORS(&pir_sensor, &vib_sensor, &button_sensor);

/*---------------------------------------------------------------------------*/
int
main(void)
{
  uip_ipaddr_t addr;

  process_init();

  procinit_init();
  
  autostart_start(autostart_processes);
  
  uip_ipaddr(&addr, 192,168,2,2);
  uip_sethostaddr(&addr);

  uip_ipaddr(&addr, 192,168,2,1);
  uip_setdraddr(&addr);

  uip_ipaddr(&addr, 255,255,255,0);
  uip_setnetmask(&addr);

  printf("Contiki initiated, now starting process scheduling\n");
  
  while(1) {
    int n;
    n = process_run();
    usleep(1);
    etimer_request_poll();
  }
  
  return 0;
}
/*---------------------------------------------------------------------------*/
void log_message(char *m1, char *m2)
{
  printf("%s%s\n", m1, m2);
}

void
uip_log(char *m)
{
  printf("%s\n", m);
}

/*unsigned short
sensors_light1(void)
{
  static unsigned short count;
  return count++;
}
*/
