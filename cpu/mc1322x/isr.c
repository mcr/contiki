#include "utils.h"
#include "timer.h"
#include "isr.h"
#include "crm.h"

#define ISR_DEBUG 0
#if ISR_DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

__attribute__ ((section (".irq")))
__attribute__ ((interrupt("IRQ"))) 
void irq(void)
{
	int pending;

	while (pending = reg32(NIPEND)) {

	 	if(bit_is_set(pending, INT_NUM_TMR)) { 
			/* dispatch to individual timer isrs if they exist */
			/* timer isrs are responsible for determining if they
			 * caused an interrupt */
			/* and clearing their own interrupt flags */
	 		if(tmr0_isr != 0) { tmr0_isr(); } 
	 		if(tmr1_isr != 0) { tmr1_isr(); } 
	 		if(tmr2_isr != 0) { tmr2_isr(); } 
	 		if(tmr3_isr != 0) { tmr3_isr(); } 
	 	}
		if(bit_is_set(pending, INT_NUM_MACA)) {
	 		if(maca_isr != 0) { maca_isr(); } 
		}
		if(bit_is_set(pending, INT_NUM_UART1)) {
	 		if(uart1_isr != 0) { uart1_isr(); } 
		}
		if(bit_is_set(pending, INT_NUM_CRM)) {
			PRINTF("crm irq\n\r");
			if(rtc_wu_evt() && (rtc_isr != 0)) { rtc_isr(); }
			if(kbi_evnt(4) && (kbi4_isr != 0)) { kbi4_isr(); }
			if(kbi_evnt(5) && (kbi5_isr != 0)) { kbi5_isr(); }
			if(kbi_evnt(6) && (kbi6_isr != 0)) { kbi6_isr(); }
			if(kbi_evnt(7) && (kbi7_isr != 0)) { kbi7_isr(); }
		}
	}
}
