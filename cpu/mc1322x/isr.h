#ifndef ISR_H
#define ISR_H

#include "utils.h"

#define INTBASE (0x80020000)
#define INTENNUM_OFF (0x8)
#define INTSRC_OFF (0x30)

#define INTENNUM INTBASE + INTENNUM_OFF
#define INTSRC   INTBASE + INTSRC_OFF 

enum interrupt_nums {
	INT_NUM_ASM = 0,
	INT_NUM_UART1,
	INT_NUM_UART2,
	INT_NUM_CRM,
	INT_NUM_I2C,
	INT_NUM_TMR,
	INT_NUM_SPIF,
	INT_NUM_MACA,
	INT_NUM_SSI,
	INT_NUM_ADC,
	INT_NUM_SPI,
};

#define enable_irq(irq) (reg32(INTENNUM) = INT_NUM_##irq)

#define tmr_irq() (bit_is_set(reg32(INTSRC),INT_NUM_TMR))

extern void tmr0_isr(void) __attribute__((weak));
extern void tmr1_isr(void) __attribute__((weak));
extern void tmr2_isr(void) __attribute__((weak));
extern void tmr3_isr(void) __attribute__((weak));

#endif

