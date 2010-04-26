#include "contiki-conf.h"
#include "dev/leds.h"
#include "mc1322x.h"
#include "board.h"

#define LED_ARCH_RED   (1ULL << LED_RED)
#define LED_ARCH_GREEN (1ULL << LED_GREEN)
#define LED_ARCH_BLUE  (1ULL << LED_BLUE)

#define LED_ARCH_YELLOW (LED_ARCH_RED  | LED_ARCH_GREEN           )
#define LED_ARCH_PURPLE (LED_ARCH_RED  |             LED_ARCH_BLUE)
#define LED_ARCH_CYAN   (           LED_ARCH_GREEN | LED_ARCH_BLUE)
#define LED_ARCH_WHITE  (LED_ARCH_RED  | LED_ARCH_GREEN | LED_ARCH_BLUE)

void leds_arch_init(void)
{
	gpio_pad_dir(LED_ARCH_WHITE);
}

unsigned char leds_arch_get(void)
{
	uint64_t led = (((uint64_t)*GPIO_DATA1) << 32) | *GPIO_DATA0;

	return ((led & LED_ARCH_RED) ? 0 : LEDS_RED)
		| ((led & LED_ARCH_GREEN) ? 0 : LEDS_GREEN)
		| ((led & LED_ARCH_BLUE) ? 0 : LEDS_BLUE)
		| ((led & LED_ARCH_YELLOW) ? 0 : LEDS_YELLOW);

}

void leds_arch_set(unsigned char leds)
{
	uint64_t led;

	led = (led & ~(LED_ARCH_RED|LED_ARCH_GREEN|LED_ARCH_YELLOW|LED_ARCH_BLUE))
		| ((leds & LEDS_RED) ? LED_ARCH_RED : 0)
		| ((leds & LEDS_GREEN) ? LED_ARCH_GREEN : 0)
		| ((leds & LEDS_BLUE) ? LED_ARCH_BLUE : 0)
		| ((leds & LEDS_YELLOW) ? LED_ARCH_YELLOW : 0);

	gpio_data(led);
}

