#include "utils.h"
#include "timer.h"
#include "isr.h"
#include "crm.h"

__attribute__ ((section (".irq")))
__attribute__ ((interrupt("IRQ"))) 
void irq(void)
{
	volatile uint32_t tmp;
 	if(tmr_irq()) { 
		/* dispatch to individual timer isrs if they exist */
		/* timer isrs are responsible for determining if they
		 * caused an interrupt */
		/* and clearing their own interrupt flags */
 		if(tmr0_isr != 0) { tmr0_isr(); } 
 		if(tmr1_isr != 0) { tmr1_isr(); } 
 		if(tmr2_isr != 0) { tmr2_isr(); } 
 		if(tmr3_isr != 0) { tmr3_isr(); } 
 	}
	if(crm_irq()) {
		if(kbi_evnt(4) && (kbi4_isr != 0)) { kbi4_isr(); }
		if(kbi_evnt(5) && (kbi5_isr != 0)) { kbi5_isr(); }
		if(kbi_evnt(6) && (kbi6_isr != 0)) { kbi6_isr(); }
		if(kbi_evnt(7) && (kbi7_isr != 0)) { kbi7_isr(); }
	}


}
