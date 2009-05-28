#include "crm.h"
#include "maca.h"

void sleep(uint32_t opts, uint32_t mode)
{

	/* the maca must be off before going to sleep */
	/* otherwise the mcu will reboot on wakeup */
	maca_off();
	reg32(CRM_SLEEP_CNTL) = opts;
	reg32(CRM_SLEEP_CNTL) = (opts | mode);

	/* wait for the sleep cycle to complete */
	while(!bit_is_set(reg32(CRM_STATUS),0)) { continue; }
        /* write 1 to sleep_sync --- this clears the bit (it's a r1wc bit) and powers down */
	set_bit(reg32(CRM_STATUS),0);

	/* now we are asleep */
	/* and waiting for something to wake us up */
	/* you did tell us how to wake up right? */

	/* waking up */
	while(!bit_is_set(reg32(CRM_STATUS),0)) { continue; }
        /* write 1 to sleep_sync --- this clears the bit (it's a r1wc bit) and finishes the wakeup */
	set_bit(reg32(CRM_STATUS),0);

	/* you may also need to do other recovery */
	/* such as interrupt handling */
	/* peripheral init */
	/* and turning the radio back on */

}

/* turn on the 32kHz crystal */
/* once you start the 32xHz crystal it can only be stopped with a reset (hard or soft) */
void enable_32khz_xtal(void) 
{
	static volatile uint32_t rtc_count;
	/* first, disable the ring osc */
	ring_osc_off();
	/* enable the 32kHZ crystal */
	xtal32_on();

        /* set the XTAL32_EXISTS bit */
        /* the datasheet says to do this after you've check that RTC_COUNT is changing */
        /* the datasheet is not correct */
	xtal32_exists();

	/* now check that the crystal starts */
	/* this blocks until it starts */
	/* it would be better to timeout and return an error */
	rtc_count = reg32(CRM_RTC_COUNT);
	while(reg32(CRM_RTC_COUNT) == rtc_count) {
		continue;
	}
	/* RTC has started up */

}
