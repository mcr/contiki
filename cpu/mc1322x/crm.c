#include "crm.h"
#include "maca.h"

void sleep(uint32_t opts, uint32_t mode)
{

	/* the maca must be off before going to sleep */
	/* maca_sleep(0) will make sure the maca is off */
	/* maca_sleep(1) will restore the maca power to the state it was at */
	/* when maca_sleep(0) was called */
//	maca_sleep(0);
	maca_off();
//	reg32(CRM_SLEEP_CNTL) = opts;
//	reg32(CRM_SLEEP_CNTL) = (opts | mode);

	reg32(CRM_SLEEP_CNTL) = 0x60;
	reg32(CRM_SLEEP_CNTL) = 0x61;

	/* wait for the sleep cycle to complete */
//	while(!bit_is_set(reg32(CRM_STATUS),0)) { continue; }	
	while((reg32(CRM_STATUS) & 0x1) == 0) { continue; }
        /* write 1 to sleep_sync --- this clears the bit (it's a r1wc bit) and powers down */	
	set_bit(reg32(CRM_STATUS),0); 

	/* now we are asleep */
	/* and waiting for something to wake us up */
	/* you did tell us how to wake up right? */

	/* waking up */
//	while(!bit_is_set(reg32(CRM_STATUS),0)) { continue; }	
	while((reg32(CRM_STATUS) & 0x1) == 0) { continue; }
        /* write 1 to sleep_sync --- this clears the bit (it's a r1wc bit) and finishes the wakeup */	
	set_bit(reg32(CRM_STATUS),0); 

	/* restore the maca_power */
//	maca_sleep(1);
	maca_on();

	/* you may also need to do other recovery */
	/* such as interrupt handling */
	/* and peripheral init */

}
