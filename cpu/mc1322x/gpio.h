#ifndef GPIO_H
#define GPIO_H

#define GPIO_PAD_DIR0      0x80000000
#define GPIO_DATA0         0x80000008
#define GPIO_FUNC_SEL0     0x80000018 /* GPIO 15 - 0;  2 bit blocks */
#define GPIO_PAD_PU_SEL0   0x80000030
#define GPIO_PAD_PU_SEL1   0x80000034

/* select pullup or pulldown for GPIO 0-31 (b=0-31) */
#define	gpio_sel0_pullup(b) (set_bit(reg32(GPIO_PAD_PU_SEL0),b))
#define	gpio_sel0_pulldown(b) (clear_bit(reg32(GPIO_PAD_PU_SEL0),b))

/* select pullup or pulldown for GPIO 32-63 (b=32-63) */
#define	gpio_sel1_pullup(b) (set_bit(reg32(GPIO_PAD_PU_SEL1),b-32))
#define	gpio_sel1_pulldown(b) (clear_bit(reg32(GPIO_PAD_PU_SEL1),b-32))

#endif
