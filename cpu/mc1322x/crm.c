#include "crm.h"
#include "maca.h"

#define CRM_DEBUG 1
#if CRM_DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint32_t cal_rtc_secs;      /* calibrated 2khz rtc seconds */

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
        PRINTF("enabling 32kHz crystal\n\r");
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
	PRINTF("waiting for xtal\n\r");
	while(reg32(CRM_RTC_COUNT) == rtc_count) {
		continue;
	}
	/* RTC has started up */
	PRINTF("32kHZ xtal started\n\r");

}

void cal_ring_osc(void)
{
	uint32_t cal_factor;
	PRINTF("performing ring osc cal\n\r");
	PRINTF("crm_status: 0x%0x\n\r",reg32(CRM_STATUS));
	PRINTF("sys_cntl: 0x%0x\n\r",reg32(CRM_SYS_CNTL)); 
	reg32(CRM_CAL_CNTL) = (1<<16) | (20000); 
	while((reg32(CRM_STATUS) & (1<<9)) == 0);
	PRINTF("ring osc cal complete\n\r");
	PRINTF("cal_count: 0x%0x\n\r",reg32(CRM_CAL_COUNT)); 
	cal_factor = (REF_OSC*1000)/reg32(CRM_CAL_COUNT);
	cal_rtc_secs = (NOMINAL_RING_OSC_SEC * cal_factor)/100;
	PRINTF("cal factor: %d \n\nr", cal_factor);
	PRINTF("hib_wake_secs: %d \n\nr", cal_rtc_secs);      
	set_bit(reg32(CRM_STATUS),9);
}
