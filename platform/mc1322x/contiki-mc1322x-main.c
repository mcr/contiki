/*
 * Copyright (c) 2006, Technical University of Munich
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
 * @(#)$$
 */

#include <stdio.h>

#include <stdbool.h>

/* contiki */
#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"

#include "net/mac/frame802154.h"
#include "net/mac/nullmac.h"
#include "net/mac/xmac.h"
#include "net/mac/lpp.h"

#include "net/rime.h"
#include "net/rime/rimeaddr.h"
#include "net/rime/ctimer.h"

/* mc1322x */
#include "isr.h"
#include "gpio.h"
#include "uart1.h"
#include "maca.h"
#include "nvm.h"
#include "kbi.h"


#ifndef RIMEADDR_NVM
#define RIMEADDR_NVM 0x1E000
#endif

#ifndef RIMEADDR_NBYTES
#define RIMEADDR_NBYTES 8
#endif

void
init_lowlevel(void)
{
	/* led direction init */
	set_bit(reg32(GPIO_PAD_DIR0),8);
	set_bit(reg32(GPIO_PAD_DIR0),9);
	set_bit(reg32(GPIO_PAD_DIR0),10);

	/* button init */
	/* set up kbi */
//	enable_irq_kbi(7);
	kbi_edge(7);
	enable_ext_wu(7);
	kbi_pol_neg(7);
//	kbi_pol_pos(7);
//	gpio_sel0_pullup(29);
	gpio_pu0_disable(29);
	
	/* uart init */
	uart1_init();
	
	enable_irq(CRM);

	/* radio init */
	reset_maca();
	/* it looks like radio_init does something to */
	/* that changes the ring_oscillator           */
	radio_init();
	vreg_init();
	flyback_init();
	init_phy();
	
	set_power(0x0f); /* 0dbm */
//	set_power(0x0); 
	set_channel(0); /* channel 11 */

#if USE_32KHZ_XTAL
	enable_32khz_xtal();
#else
	cal_ring_osc();
#endif

#if USE_32KHZ_XTAL
	reg32(CRM_RTC_TIMEOUT) = 32768 * 10; 
#else 
	reg32(CRM_RTC_TIMEOUT) = cal_rtc_secs * 10;
#endif

	/* XXX debug */
	/* trigger periodic rtc int */
	clear_rtc_wu_evt();
	enable_rtc_wu();
	enable_rtc_wu_irq();
}

void
set_rimeaddr(rimeaddr_t *addr) 
{
	nvm_type_t type=0;
	nvm_err_t err;	
	volatile uint8_t buf[RIMEADDR_NBYTES];
	int i;

	err = nvm_detect(NVM_INTERFACE_INTERNAL, &type);

	err = nvm_read(NVM_INTERFACE_INTERNAL, type, (uint8_t *)buf, RIMEADDR_NVM, RIMEADDR_NBYTES);

	rimeaddr_copy(addr,&rimeaddr_null);

	for(i=0; i<RIMEADDR_CONF_SIZE; i++) {		
		addr->u8[i] = buf[i];
	}

	rimeaddr_set_node_addr(addr);
}

//PROCESS_NAME(blink8_process);
//PROCESS_NAME(blink9_process);
//PROCESS_NAME(blink10_process);
//PROCESS_NAME(maca_process);

//PROCINIT(&etimer_process, &blink8_process,&blink9_process,&blink10_process);
//PROCINIT(&etimer_process, &blink8_process, &blink9_process, &blink10_process);
//PROCINIT(&etimer_process, &ctimer_process, &maca_process, &blink8_process,&blink9_process,&blink10_process);
PROCINIT(&etimer_process, &ctimer_process, &maca_process);
//AUTOSTART_PROCESSES(&etimer_process, &blink8_process, &blink9_process, &blink10_process);
//AUTOSTART_PROCESSES(&hello_world_process);

int
main(void)
{
	volatile uint32_t i;
	rimeaddr_t addr;

	/* Clock */
	clock_init();
	
	/* Initialize hardware and */
	/* go into user mode */
	init_lowlevel();

	/* Process subsystem */
	process_init();

	/* Register initial processes */
	procinit_init();

	//rime_init(nullmac_init(&maca_driver));
	rime_init(xmac_init(&maca_driver));

	set_rimeaddr(&addr);

	printf("Rime started with address ");
	for(i = 0; i < sizeof(addr.u8) - 1; i++) {
		printf("%d.", addr.u8[i]);
	}
	printf("%d\n", addr.u8[i]);

	/* Autostart processes */
	autostart_start(autostart_processes);
	
	//Give ourselves a prefix
	//init_net();
	
	printf("\n\r********BOOTING CONTIKI*********\n\r");
	
	printf("System online.\n\r");
	
	/* Main scheduler loop */
	while(1) {
		process_run();
	}
	
	return 0;
}
