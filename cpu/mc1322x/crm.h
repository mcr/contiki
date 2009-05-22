#ifndef CRM_H
#define CRM_H

#define CRM_BASE         (0x80003000)
#define CRM_SYS_CNTL     (CRM_BASE+0x00)
#define CRM_WU_CNTL      (CRM_BASE+0x04)
#define CRM_SLEEP_CNTL   (CRM_BASE+0x08)
#define CRM_BS_CNTL      (CRM_BASE+0x0c)
#define CRM_COP_CNTL     (CRM_BASE+0x10)
#define CRM_COP_SERVICE  (CRM_BASE+0x14)
#define CRM_STATUS       (CRM_BASE+0x18)
#define CRM_MOD_STATUS   (CRM_BASE+0x1c)
#define CRM_WU_COUNT     (CRM_BASE+0x20)
#define CRM_WU_TIMEOUT   (CRM_BASE+0x24)
#define CRM_RTC_COUNT    (CRM_BASE+0x28)
#define CRM_RTC_TIMEOUT  (CRM_BASE+0x2c)
#define CRM_CAL_CNTL     (CRM_BASE+0x34)
#define CRM_CAL_COUNT    (CRM_BASE+0x38)
#define CRM_RINGOSC_CNTL (CRM_BASE+0x3c)
#define CRM_XTAL_CNTL    (CRM_BASE+0x40)
#define CRM_XTAL32_CNTL  (CRM_BASE+0x44)
#define CRM_VREG_CNTL    (CRM_BASE+0x48)
#define CRM_SW_RST       (CRM_BASE+0x50)

/* wu_cntl bit locations */
#define EXT_WU_IEN   20      /* 4 bits */ 
#define EXT_WU_EN    4       /* 4 bits */ 
#define EXT_WU_EDGE  8       /* 4 bits */ 
#define EXT_WU_POL   12      /* 4 bits */ 
#define TIMER_WU_EN  0 
#define RTC_WU_EN    1 

/* status bit locations */
#define EXT_WU_EVT 4       /* 4 bits */

/* RINGOSC_CNTL bit locations */
#define ROSC_CTUNE 9       /* 4 bits */
#define ROSC_FTUNE 4       /* 4 bits */
#define ROSC_EN    0

#define ring_osc_on() (set_bit(reg32(CRM_RINGOSC_CNTL),ROSC_EN))
#define ring_osc_off() (clear_bit(reg32(CRM_RINGOSC_CNTL),ROSC_EN))

/* XTAL32_CNTL bit locations */
#define XTAL32_GAIN 4      /* 2 bits */

/* enable external wake-ups on kbi 4-7 */ 
/* see kbi.h for other kbi specific macros */
#define enable_ext_wu(kbi) (set_bit(reg32(CRM_WU_CNTL),(EXT_WU_EN+kbi-4)))
#define disable_ext_wu(kbi) (clear_bit(reg32(CRM_WU_CNTL),(EXT_WU_EN+kbi-4)))

#define is_ext_wu_evt(kbi) (bit_is_set(reg32(CRM_STATUS),(EXT_WU_EVT+kbi-4)))
#define clear_ext_wu_evt(kbi) (set_bit(reg32(CRM_STATUS),(EXT_WU_EVT+kbi-4))) /* r1wc bit */

/* enable wake-up timer */
#define enable_timer_wu() ((set_bit(reg32(CRM_WU_CNTL),(TIMER_WU_EN))))
#define disable_timer_wu() ((clear_bit(reg32(CRM_WU_CNTL),(TIMER_WU_EN))))

/* enable wake-up from RTC compare */
#define enable_rtc_wu() ((set_bit(reg32(CRM_WU_CNTL),(RTC_WU_EN))))
#define disable_rtc_wu() ((clear_bit(reg32(CRM_WU_CNTL),(RTC_WU_EN))))

#define SLEEP_MODE_HIBERNATE bit(0)
#define SLEEP_MODE_DOZE      bit(1)

#define SLEEP_PAD_PWR     bit(7)
#define SLEEP_RETAIN_MCU bit(6)
#define sleep_ram_retain(x) (x<<4)   /* 0-3 */
#define SLEEP_RAM_8K sleep_ram_retain(0)
#define SLEEP_RAM_32K sleep_ram_retain(1)
#define SLEEP_RAM_64K sleep_ram_retain(2)
#define SLEEP_RAM_96K sleep_ram_retain(3)

#endif
