#include "lib/sensors.h"
#include "dev/button-sensor.h"

#include "mc1322x.h"

#include <signal.h>

const struct sensors_sensor button_sensor;

static struct timer debouncetimer;
static int status(int type);

void kbi4_isr(void) {
	if(timer_expired(&debouncetimer)) {
		timer_set(&debouncetimer, CLOCK_SECOND / 4);
		sensors_changed(&button_sensor);
	}
	clear_kbi_evnt(4);
}

static int
value(int type)
{
	return bit_is_set(gpio_data_get((0x1ULL << 26)), 26) || !timer_expired(&debouncetimer);
}

static int
configure(int type, int c)
{
	switch (type) {
	case SENSORS_ACTIVE:
		if (c) {
			if(!status(SENSORS_ACTIVE)) {
				timer_set(&debouncetimer, 0);
				enable_irq_kbi(4);
			}
		} else {
			disable_irq_kbi(4);
		}
		return 1;
	}
	return 0;
}

static int
status(int type)
{
	switch (type) {
	case SENSORS_ACTIVE:
	case SENSORS_READY:
		return bit_is_set(*CRM_WU_CNTL, 20); /* check if kbi4 irq is enabled */
	}
	return 0;
}

SENSORS_SENSOR(button_sensor, BUTTON_SENSOR,
	       value, configure, status);
