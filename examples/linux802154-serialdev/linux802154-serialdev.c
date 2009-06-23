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
 * This file is part of the Contiki operating system.
 *
 * $Id: hello-world.c,v 1.1 2006/10/02 21:46:46 adamdunkels Exp $
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "rime.h"

#include "linux-serialdev.h"
#define start_command() ((sb[0] == START_BYTE1) && (sb[1] == START_BYTE2))

#include "gpio.h"
#include "maca.h"

#define led_red_on() do { set_bit(reg32(GPIO_DATA0),8); set_bit(reg32(GPIO_DATA0),23); } while (0)
#define led_red_off() do { clear_bit(reg32(GPIO_DATA0),8); clear_bit(reg32(GPIO_DATA0),23); } while (0)

#define led_green_on() do { set_bit(reg32(GPIO_DATA0),9); set_bit(reg32(GPIO_DATA0),24); } while (0)
#define led_green_off() do { clear_bit(reg32(GPIO_DATA0),9); clear_bit(reg32(GPIO_DATA0),24); } while(0)

#define led_blue_on() do { set_bit(reg32(GPIO_DATA0),10); set_bit(reg32(GPIO_DATA0),25); } while (0)
#define led_blue_off() do { clear_bit(reg32(GPIO_DATA0),10); clear_bit(reg32(GPIO_DATA0),25); } while(0)

#include <stdio.h> /* For printf() */

static volatile uint8_t buf[MAX_DATA_SIZE];

void di(const struct radio_driver * radio) {
	uint8_t len,i;
	len = maca_driver.read((uint8_t *)buf,MAX_DATA_SIZE);
	printf("zb");
	uart1_putchar(DATA_RECV_BLOCK);
	uart1_putchar(0);
	uart1_putchar(len);
	for(i=0; i<len; i++) {
		uart1_putchar(buf[i]);
	}
}	


/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
	uint8_t sb[NUM_START_BYTES];
	volatile uint32_t i;
	uint8_t cmd,parm1,parm2;
	static uint8_t state = IDLE_MODE;

	PROCESS_BEGIN();

	maca_driver.set_receive_function(di);

	while(1) {
		
		/* clear out sb */
		for(i=0; i<NUM_START_BYTES; i++) {
			sb[i] = 0;
		}

		/* recieve bytes until we see the first start byte */
		/* this syncs up to the commands */
		while(sb[0] != START_BYTE1) {
			while(!uart1_can_get()) { PROCESS_PAUSE();}
			sb[0] = uart1_getc();
		}

		/* receive start bytes */
		for(i=1; i<NUM_START_BYTES; i++) {
			while(!uart1_can_get()) { /* PROCESS_PAUSE(); */};
			sb[i] = uart1_getc();
		}
		
		if(start_command()) {
			/* do a command */
			cmd = 0;
//			led_blue_on();

			while(!uart1_can_get()) { /* PROCESS_PAUSE(); */};
			cmd = uart1_getc();
			
			switch(cmd)
			{
			case CMD_OPEN:
				printf("zb");
				uart1_putchar(RESP_OPEN);
				uart1_putchar(STATUS_SUCCESS);
				break;
			case CMD_CLOSE:
				printf("zb");
				uart1_putchar(RESP_CLOSE);
				uart1_putchar(STATUS_SUCCESS);
				break;
			case CMD_SET_CHANNEL:
				parm1 = uart1_getc();
				set_channel(parm1);
				printf("zb");
				uart1_putchar(RESP_SET_CHANNEL);
				uart1_putchar(STATUS_SUCCESS);
				break;
			case CMD_ED:
				printf("zb");
				uart1_putchar(RESP_ED);
				uart1_putchar(STATUS_SUCCESS);
				uart1_putchar(0);
				break;
			case CMD_SET_STATE:
				state = uart1_getc();
				printf("zb");
				uart1_putchar(RESP_SET_STATE);
				uart1_putchar(STATUS_SUCCESS);
			case DATA_XMIT_BLOCK:
				parm1 = uart1_getc();
				for(i=0; i<parm1; i++) {
					buf[i] = uart1_getc();
				}
				maca_driver.send((const uint8_t *)buf,parm1);
				printf("zb");
				uart1_putchar(RESP_XMIT_BLOCK);
				uart1_putchar(STATUS_SUCCESS);
			default:
				break;
			}

		}
		
		PROCESS_PAUSE();

	}

	
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
