#include <sys/clock.h>
#include <sys/cc.h>
#include <sys/etimer.h>

#include "utils.h"
#include "timer.h"
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

	reg16(TMR_ENBL) = 0;                     /* tmrs reset to enabled */
	reg16(TMR0_SCTRL) = 0;
	reg16(TMR0_CSCTRL) =0x0040;
	reg16(TMR0_LOAD) = 0;                    /* reload to zero */
	reg16(TMR0_COMP_UP) = 18750;             /* trigger a reload at the end */
	reg16(TMR0_CMPLD1) = 18750;              /* compare 1 triggered reload level, 10HZ maybe? */
	reg16(TMR0_CNTR) = 0;                    /* reset count register */
	reg16(TMR0_CTRL) = (COUNT_MODE<<13) | (PRIME_SRC<<9) | (SEC_SRC<<7) | (ONCE<<6) | (LEN<<5) | (DIR<<4) | (CO_INIT<<3) | (OUT_MODE);
	reg16(TMR_ENBL) = 0xf;                   /* enable all the timers --- why not? */

	enable_irq(TMR);

}

volatile uint8_t led;

#define GPIO_DATA0      0x80000008
#define led_on() do  { led = 1; reg32(GPIO_DATA0) = 0x00000100; } while(0);
#define led_off() do { led = 0; reg32(GPIO_DATA0) = 0x00000000; } while(0);

void toggle_led(void) {
	if(0 == led) {
		led_on();
		led = 1;

	} else {
		led_off();
	}
}


void tmr0_isr(void) {
	if(bit_is_set(reg16(TMR(0,CSCTRL)),TCF1)) {
		current_clock++;
		if(etimer_pending() && etimer_next_expiration_time() <= current_clock) {
			etimer_request_poll();
			/* dbg_printf("%d,%d\n", clock_time(),etimer_next_expiration_time  	()); */
			
		}
		toggle_led();
		/* clear the compare flags */
		reg16(TMR(0,SCTRL))  = clear_bit(reg16(TMR(0,SCTRL)),TCF);                
		reg16(TMR(0,CSCTRL)) = clear_bit(reg16(TMR(0,CSCTRL)),TCF1);                
		reg16(TMR(0,CSCTRL)) = clear_bit(reg16(TMR(0,CSCTRL)),TCF2);                
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

