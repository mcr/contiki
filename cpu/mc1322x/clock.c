#include <sys/clock.h>
#include <sys/cc.h>
#include <sys/etimer.h>

#include "utils.h"
#include "tmr.h"
#include "isr.h"

static volatile clock_time_t current_clock = 0;

void
clock_init()
{
	/* timer setup */
	/* CTRL */
#define COUNT_MODE 1      /* use rising edge of primary source */
#define PRIME_SRC  0xf    /* Perip. clock with 128 prescale (for 24Mhz = 187500Hz)*/
#define SEC_SRC    0      /* don't need this */
#define ONCE       0      /* keep counting */
#define LEN        1      /* count until compare then reload with value in LOAD */
#define DIR        0      /* count up */
#define CO_INIT    0      /* other counters cannot force a re-initialization of this counter */
#define OUT_MODE   0      /* OFLAG is asserted while counter is active */

	*TMR_ENBL = 0;                     /* tmrs reset to enabled */
	*TMR0_SCTRL = 0;
	*TMR0_CSCTRL =0x0040;
	*TMR0_LOAD = 0;                    /* reload to zero */
	*TMR0_COMP_UP = 1875;             /* trigger a reload at the end */
	*TMR0_CMPLD1 = 1875;              /* compare 1 triggered reload level, 10HZ maybe? */
	*TMR0_CNTR = 0;                    /* reset count register */
	*TMR0_CTRL = (COUNT_MODE<<13) | (PRIME_SRC<<9) | (SEC_SRC<<7) | (ONCE<<6) | (LEN<<5) | (DIR<<4) | (CO_INIT<<3) | (OUT_MODE);
	*TMR_ENBL = 0xf;                   /* enable all the timers --- why not? */

	enable_irq(TMR);

}

#define GPIO_DATA0      0x80000008

static volatile uint8_t tmr_led,tmr_led9=0;

void tmr0_isr(void) {
	if(bit_is_set(*TMR(0,CSCTRL),TCF1)) {
		current_clock++;
		/* maybe blink out current clock somehow for debug?*/
		/* maybe check if a bit is set in current_clock? */
		/* that should give me a divided blink */
		
//		if(etimer_pending() && etimer_next_expiration_time() <= current_clock) {
		if(etimer_pending()) {
			etimer_request_poll();
			/* dbg_printf("%d,%d\n", clock_time(),etimer_next_expiration_time  	()); */			

			if(tmr_led == 0) {
//				set_bit(reg32(GPIO_DATA0),10);
				tmr_led = 1;
			} else {
//				clear_bit(reg32(GPIO_DATA0),10);
				tmr_led = 0;
			}

		}

/* 		if((current_clock % 32) == 0) { */
/* 			if(tmr_led9 == 0) { */
/* 				set_bit(reg32(GPIO_DATA0),9); */
/* 				tmr_led9 = 1; */
/* 			} else { */
/* 				clear_bit(reg32(GPIO_DATA0),9); */
/* 				tmr_led9 = 0; */
/* 			} */
/* 		} */

		/* clear the compare flags */
		clear_bit(*TMR(0,SCTRL),TCF);                
		clear_bit(*TMR(0,CSCTRL),TCF1);                
		clear_bit(*TMR(0,CSCTRL),TCF2);                
		return;
	} else {
		/* this timer didn't create an interrupt condition */
		return;
	}
}

clock_time_t
clock_time(void)
{
  return current_clock;
}

