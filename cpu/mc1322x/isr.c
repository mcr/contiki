#include "isr.h"

__attribute__ ((section (".irq")))
__attribute__ ((interrupt("IRQ"))) 
void irq(void)
{
	if(tmr_isr != 0) {
		tmr_isr(); 
	}
}
