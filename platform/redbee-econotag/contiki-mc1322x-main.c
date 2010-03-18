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
#include "net/mac/sicslowmac.h"

#include "net/rime.h"
#include "net/rime/rimeaddr.h"
#include "net/rime/ctimer.h"

/* from libmc1322x */
#include "mc1322x.h"
#include "default_lowlevel.h"

#include "contiki-maca.h"

#ifndef RIMEADDR_NVM
#define RIMEADDR_NVM 0x1E000
#endif

#ifndef RIMEADDR_NBYTES
#define RIMEADDR_NBYTES 8
#endif

#define PLATFORM_DEBUG 1
#if PLATFORM_DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void
init_lowlevel(void)
{
	/* led direction init */
	set_bit(*GPIO_PAD_DIR0,8);
	set_bit(*GPIO_PAD_DIR0,9);
	set_bit(*GPIO_PAD_DIR0,10);
	set_bit(*GPIO_PAD_DIR0,23);
	set_bit(*GPIO_PAD_DIR0,24);
	set_bit(*GPIO_PAD_DIR0,25);

	/* button init */
	/* set up kbi */
//	enable_irq_kbi(7);
	kbi_edge(7);
	enable_ext_wu(7);
	kbi_pol_neg(7);
//	kbi_pol_pos(7);
//	gpio_sel0_pullup(29);
	gpio_pu0_disable(29);

	trim_xtal();
	
	/* uart init */
	uart_init(INC, MOD, SAMP);
	
	default_vreg_init();

	maca_init();

	set_channel(0); /* channel 11 */
	set_power(0x12); /* 0x12 is the highest, not documented */

        *GPIO_FUNC_SEL2 = (0x01 << ((44-16*2)*2));
	gpio_pad_dir_set( 1ULL << 44 );

	enable_irq(CRM);

#if USE_32KHZ_XTAL
	enable_32khz_xtal();
#else
	cal_ring_osc();
#endif

#if USE_32KHZ_XTAL
	*CRM_RTC_TIMEOUT = 32768 * 10; 
#else 
	*CRM_RTC_TIMEOUT = cal_rtc_secs * 10;
#endif

	/* XXX debug */
	/* trigger periodic rtc int */
//	clear_rtc_wu_evt();
//	enable_rtc_wu();
//	enable_rtc_wu_irq();
}

void
set_rimeaddr(rimeaddr_t *addr) 
{
	nvmType_t type=0;
	nvmErr_t err;	
	volatile uint8_t buf[RIMEADDR_NBYTES];
	int i;

	err = nvm_detect(gNvmInternalInterface_c, &type);

	err = nvm_read(gNvmInternalInterface_c, type, (uint8_t *)buf, RIMEADDR_NVM, RIMEADDR_NBYTES);

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
//PROCINIT(&etimer_process, &ctimer_process);
//AUTOSTART_PROCESSES(&etimer_process, &blink8_process, &blink9_process, &blink10_process);
//AUTOSTART_PROCESSES(&hello_world_process);

int
main(void)
{
	volatile uint32_t i;
	rimeaddr_t addr;

	/* Initialize hardware and */
	/* go into user mode */
	init_lowlevel();

	/* Clock */
	clock_init();	

	/* Process subsystem */
	process_init();

	/* Register initial processes */
	procinit_init();

//	rime_init(nullmac_init(&maca_driver));
//	rime_init(xmac_init(&maca_driver));
//	rime_init(lpp_init(&maca_driver));
	rime_init(sicslowmac_init(&maca_driver));

#if !(USE_32KHZ_XTAL)
	PRINTF("setting xmac to use calibrated rtc value\n\r");
	xmac_config.on_time = cal_rtc_secs/200;
	xmac_config.off_time = cal_rtc_secs/2 - xmac_config.on_time;
	xmac_config.strobe_time = 20 * xmac_config.on_time + xmac_config.off_time;
	xmac_config.strobe_wait_time = 7 * xmac_config.on_time / 8;
	PRINTF("xmac_config.on_time %u\n\r",xmac_config.on_time);
	PRINTF("xmac_config.off_time %u\n\r",xmac_config.off_time);
	PRINTF("xmac_config.strobe_time %u\n\r",xmac_config.strobe_time);
	PRINTF("xmac_config.strobe_wait_time %u\n\r",xmac_config.strobe_wait_time);
#endif


//	set_rimeaddr(&addr);

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
