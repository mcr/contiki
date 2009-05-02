#ifndef _MACA_H_
#define _MACA_H_

#include <stdint.h>

/* #define gMACA_Clock_DIV_c      95 */

/* enum { */
/*    cc_success          = 0, */
/*    cc_timeout          = 1, */
/*    cc_channel_busy     = 2, */
/*    cc_crc_fail         = 3, */
/*    cc_aborted          = 4, */
/*    cc_no_ack           = 5, */
/*    cc_no_data          = 6, */
/*    cc_late_start       = 7, */
/*    cc_ext_timeout      = 8, */
/*    cc_ext_pnd_timeout  = 9, */
/*    cc_nc1              = 10, */
/*    cc_nc2              = 11, */
/*    cc_nc3              = 12, */
/*    cc_cc_external_abort= 13, */
/*    cc_not_completed    = 14, */
/*    cc_bus_error        = 15 */
/* }; */
/* //control codes for mode bits */

/* enum { */
/*    control_mode_no_cca      = 0, */
/*    control_mode_non_slotted = (1<<3), */
/*    control_mode_slotted     = (1<<4) */
/* }; */
/* //control codes for sequence bits */
/* enum { */
/*     control_seq_nop    = 0, */
/*     control_seq_abort  = 1, */
/*     control_seq_wait   = 2, */
/*     control_seq_tx     = 3, */
/*     control_seq_rx     = 4, */
/*     control_seq_txpoll = 5, */
/*     control_seq_cca    = 6, */
/*     control_seq_ed     = 7 */
/* };   */

/* #define maca_status_cc_mask           (0x0F) */

//#define maca_reset_rst                (1<<0)
/* #define maca_reset_cln_on             (1<<1) */

/* #define maca_frmpnd_data_pending      (1<<0) */
/* #define maca_frmpnd_no_data_pending   (0x00) */

/* #define maca_txlen_max_rxlen          (127<<16) */

/* #define max_rx_ackwnd_slotted_mode    (0xFFF<<16) */
/* #define max_rx_ackwnd_normal_mode     (0xFFF) */


/* #define control_pre_count (7<<16)   /\* preamble reapeat counter       *\/ */
/* #define control_rst_slot  (1<<15)   /\* reset slot counter             *\/ */
/* #define control_role      (1<<13)   /\* set if PAN coordinator         *\/ */
/* #define control_nofc      (1<<12)   /\* set to disable FCS             *\/ */
/* #define control_prm       (1<<11)   /\* set for promiscuous mode       *\/ */
/* #define control_relative  (1<<10)   /\* 1 for relative, 0 for absolute *\/ */
/* #define control_asap      (1<<9)    /\* 1 start now, 0 timer start     *\/ */
/* #define control_bcn       (1<<8)    /\* 1 beacon only, 0 for a         *\/ */
/* #define control_auto      (1<<7)    /\* 1 continuous rx, rx only once  *\/ */
/* #define control_lfsr      (1<<6)    /\* 1 use polynomial for Turbolink *\/ */

#endif // _MACA_H_
