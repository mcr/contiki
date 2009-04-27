#include "utils.h"
#include "timer.h"
#include "isr.h"


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


__attribute__ ((section (".irq")))
__attribute__ ((interrupt("IRQ"))) 
void irq(void)
{
	toggle_led();
	reg16(TMR0_SCTRL) = 0;
	reg16(TMR0_CSCTRL) = 0x0040; /* clear compare flag */

/* 	if(tmr_irq()) { */
/* 		/\* dispatch to individual timer isrs if they exist *\/ */
/* 		/\* timer isrs are responsible for determining if they */
/* 		 * caused an interrupt *\/ */
/* 		/\* and clearing their own interrupt flags *\/ */
/* 		if(tmr0_isr != 0) { tmr0_isr(); } */
/* 		if(tmr1_isr != 0) { tmr1_isr(); } */
/* 		if(tmr2_isr != 0) { tmr2_isr(); } */
/* 		if(tmr3_isr != 0) { tmr3_isr(); } */
/* 	} */
}
