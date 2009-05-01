#ifndef MACA_H
#define MACA_H

#include <stdint.h>

/* contiki */
#include "contiki.h"
#include "dev/radio.h"
#include "sys/process.h"

PROCESS_NAME(maca_process);

/* mc1322x */
#include "utils.h"

extern const struct radio_driver maca_driver;

#define MACA_BASE       (0x80004000)
#define MACA_RESET      (MACA_BASE+0x04)
#define MACA_RANDOM     (MACA_BASE+0x08)
#define MACA_CONTROL    (MACA_BASE+0x0c)
#define MACA_STATUS     (MACA_BASE+0x10)
#define MACA_FRMPND     (MACA_BASE+0x14)
#define MACA_TMREN      (MACA_BASE+0x40)
#define MACA_TMRDIS     (MACA_BASE+0x44)
#define MACA_CLK        (MACA_BASE+0x48)
#define MACA_STARTCLK   (MACA_BASE+0x4c)
#define MACA_CPLCLK     (MACA_BASE+0x50)
#define MACA_SFTCLK     (MACA_BASE+0x54)
#define MACA_CLKOFFSET  (MACA_BASE+0x58)
#define MACA_RELCLK     (MACA_BASE+0x5c)
#define MACA_CPLTIM     (MACA_BASE+0x60)
#define MACA_SLOTOFFSET (MACA_BASE+0x64)
#define MACA_TIMESTAMP  (MACA_BASE+0x68)
#define MACA_DMARX      (MACA_BASE+0x80)
#define MACA_DMATX      (MACA_BASE+0x84)
#define MACA_DMAPOLL    (MACA_BASE+0x88)
#define MACA_TXLEN      (MACA_BASE+0x8c)
#define MACA_TXSEQNR    (MACA_BASE+0x90)
#define MACA_SETRXLVL   (MACA_BASE+0x94)
#define MACA_GETRXLVL   (MACA_BASE+0x98)
#define MACA_IRQ        (MACA_BASE+0xc0)
#define MACA_CLRIRQ     (MACA_BASE+0xc4)
#define MACA_SETIRQ     (MACA_BASE+0xc8)
#define MACA_MASKIRQ    (MACA_BASE+0xcc)
#define MACA_MACPANID   (MACA_BASE+0x100)
#define MACA_MAC16ADDR  (MACA_BASE+0x104)
#define MACA_MAC64HI    (MACA_BASE+0x108)
#define MACA_MAC64LO    (MACA_BASE+0x10c)
#define MACA_FLTREJ     (MACA_BASE+0x110)
#define MACA_CLKDIV     (MACA_BASE+0x114)
#define MACA_WARMUP     (MACA_BASE+0x118)
#define MACA_PREAMBLE   (MACA_BASE+0x11c)
#define MACA_WHITESEED  (MACA_BASE+0x120)
#define MACA_FRAMESYNC0 (MACA_BASE+0x124)
#define MACA_FRAMESYNC1 (MACA_BASE+0x128)
#define MACA_TXACKDELAY (MACA_BASE+0x140)
#define MACA_RXACKDELAY (MACA_BASE+0x144)
#define MACA_EOFDELAY   (MACA_BASE+0x148)
#define MACA_CCADELAY   (MACA_BASE+0x14c)
#define MACA_RXEND      (MACA_BASE+0x150)
#define MACA_TXCCADELAY (MACA_BASE+0x154)
#define MACA_KEY3       (MACA_BASE+0x158)
#define MACA_KEY2       (MACA_BASE+0x15c)
#define MACA_KEY1       (MACA_BASE+0x160)
#define MACA_KEY0       (MACA_BASE+0x164)
#define MACA_OPTIONS    (MACA_BASE+0x180)

/* Sequence complete codes */
enum maca_complete_code {
   maca_cc_success          = 0,
   maca_cc_timeout          = 1,
   maca_cc_channel_busy     = 2,
   maca_cc_crc_fail         = 3,
   maca_cc_aborted          = 4,
   maca_cc_no_ack           = 5,
   maca_cc_no_data          = 6,
   maca_cc_late_start       = 7,
   maca_cc_ext_timeout      = 8,
   maca_cc_ext_pnd_timeout  = 9,
   maca_cc_nc1              = 10,
   maca_cc_nc2              = 11,
   maca_cc_nc3              = 12,
   maca_cc_cc_external_abort= 13,
   maca_cc_not_completed    = 14,
   maca_cc_bus_error        = 15
};

/* control sequence codes */
enum maca_ctrl_seq {
    maca_ctrl_seq_nop    = 0,
    maca_ctrl_seq_abort  = 1,
    maca_ctrl_seq_wait   = 2,
    maca_ctrl_seq_tx     = 3,
    maca_ctrl_seq_rx     = 4,
    maca_ctrl_seq_txpoll = 5,
    maca_ctrl_seq_cca    = 6,
    maca_ctrl_seq_ed     = 7
};  

/* transmission modes */
enum maca_ctrl_modes {
	maca_ctrl_mode_no_cca = 0,
	maca_ctrl_mode_non_slotted_csma_ca = 1,
	maca_ctrl_mode_slotted_csma_ca = 2,
};

/* MACA_CONTROL bits */
enum maca_ctrl_bits {
        maca_ctrl_seq    = 0,  /* 3 bits */
	maca_ctrl_mode   = 3,  /* 2 bits */
        maca_ctrl_tm     = 5, 
	maca_ctrl_lfsr   = 6,
        maca_ctrl_auto   = 7,
	maca_ctrl_bcn    = 8,
        maca_ctrl_asap   = 9,
        maca_ctrl_rel    = 10,
        maca_ctrl_prm    = 11,
        maca_ctrl_nofc   = 12,
        maca_ctrl_role   = 13,
        /* 14 reserved */
        maca_ctrl_rsto   = 15,
        maca_ctrl_pre_count = 16, /* 4 bits */
        maca_ctrl_ism    = 20,
};

/* MACA_IRQ bits */
enum maca_irqs {
	maca_irq_acpl     = 0, 
	maca_irq_poll     = 1,
	maca_irq_di       = 2,
	maca_irq_wu       = 3,
	maca_irq_rst      = 4,
	maca_irq_lvl      = 9,
	maca_irq_sftclk   = 10,
	maca_irq_flt      = 11,
	maca_irq_crc      = 12, 
	maca_irq_cm       = 13,
	maca_irq_sync     = 14,  
	maca_irq_strt     = 15,   
};

/* MACA_RESET bits */
enum maca_reset_bits {
	maca_reset_rst    = 0,
	maca_reset_clkon  = 1,
};

/* MACA_TMREN bits */
enum maca_tmren_bits {
	maca_tmren_strt   = 0,
	maca_tmren_cpl    = 1,
	maca_tmren_sft    = 2,
};

  
#define _is_action_complete_irq()    bit_is_set(reg32(MACA_IRQ),maca_irq_acpl)
#define _is_filter_failed_irq()     bit_is_set(reg32(MACA_IRQ),maca_irq_flt)
#define _is_checksum_failed_irq()   bit_is_set(reg32(MACA_IRQ),maca_irq_crc)

void reset_maca(void);
void init_phy(void);
void vreg_init(void);
void ResumeMACASync(void);
void radio_init(void);
uint32_t init_from_flash(uint32_t addr);
void set_power(uint8_t power);
void set_channel(uint8_t chan);

#endif
