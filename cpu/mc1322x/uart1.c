#include <stdint.h>
#include "uart1.h"
#include "utils.h"
#include "gpio.h"
#include "isr.h"

char tx_buf[1024];
int head, tail;

void uart1_init(void) {
	uint8_t i;
		
	/* Restore UART regs. to default */
	/* in case there is still bootloader state leftover */
	
	reg32(UART1_CON) = 0x0000c800; /* mask interrupts, 16 bit sample --- helps explain the baud rate */

	/* INC = 767; MOD = 9999 works: 115200 @ 24 MHz 16 bit sample */
	#define INC 767
	#define MOD 9999
	reg32(UART1_BR) = INC<<16 | MOD; 

	/* see Section 11.5.1.2 Alternate Modes */
	/* you must enable the peripheral first BEFORE setting the function in GPIO_FUNC_SEL */
	/* From the datasheet: "The peripheral function will control operation of the pad IF */
	/* THE PERIPHERAL IS ENABLED. */
	reg32(UART1_CON) = 0x00000003; /* enable receive and transmit */
	reg32(GPIO_FUNC_SEL0) = ( (0x01 << (14*2)) | (0x01 << (15*2)) ); /* set GPIO15-14 to UART (UART1 TX and RX)*/

	/* interrupt when 28 bytes are free */
	reg32(UT1CON) = 28;

	head = tail = 0;
}

void uart1_isr(void) {
 	while( reg32(UT1CON) != 0 ) {
		if (head == tail) {
			disable_irq(UART1);
			return;
		}
		reg32(UART1_DATA) = tx_buf[tail];
		tail++;		
		if (tail >= sizeof(tx_buf))
			tail = 0;
	}
	enable_irq(UART1);
}

int uart1_putchar(int c) {
	int h = head;
	h = head + 1;
	if (h >= sizeof(tx_buf))
		h = 0;
	if (h == tail) /* drop chars when no room */
		return;
	tx_buf[head] = c;
	head = h;

	uart1_isr();
	return c;
}

uint8_t uart1_getc(void) {
	while(!uart1_can_get()) { continue; }
	return reg32(UART1_DATA);
}
