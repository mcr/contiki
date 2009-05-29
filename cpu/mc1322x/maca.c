#include <stdint.h>
#include <stdio.h>

/* contiki */
#include "radio.h"
#include "sys/process.h"

/* mc1322x */
#include "maca.h"
#include "nvm.h"
#include "gpio.h"
#include "crm.h"

#ifndef MAX_PACKET_SIZE
#define MAX_PACKET_SIZE 127
#endif

static volatile uint8_t tx_buf[MAX_PACKET_SIZE]  __attribute__ ((aligned (4)));
static volatile uint8_t rx_buf[MAX_PACKET_SIZE]  __attribute__ ((aligned (4)));				
								

#ifndef MACA_SOFT_TIMEOUT
#define MACA_SOFT_TIMEOUT 5000
#endif

#if MACA_DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifndef MACA_RAW_PREPEND
#define MACA_RAW_PREPEND 0xff
#endif

/* contiki mac driver */

static void (* receiver_callback)(const struct radio_driver *);

int maca_on(void);
int maca_off(void);
int maca_read(void *buf, unsigned short bufsize);
int maca_send(const void *data, unsigned short len);
void maca_set_receiver(void (* recv)(const struct radio_driver *d));

const struct radio_driver maca_driver =
{
	maca_send,
	maca_read,
	maca_set_receiver,
	maca_on,
	maca_off,
};

int maca_on(void) {
	PRINTF("maca on\n\r");
#ifdef DISABLE_RECEPTION
	PRINTF("reception is disabled\n\r");
#endif /*DISABLE_RECEPTION*/	

        /* turn the radio regulators back on */
	reg32(CRM_VREG_CNTL) = reg32(CRM_VREG_CNTL) | 0x00000078;

	/* reinitialize the phy */
        init_phy();

	return 1;
}

int maca_off(void) {
	PRINTF("maca off\n\r");
        /* turn off the radio regulators */
	reg32(CRM_VREG_CNTL) = reg32(CRM_VREG_CNTL) & (~0x00000078);

        /* hold the maca in reset */
	set_bit(reg32(MACA_RESET),maca_reset_rst);
	return 1;
}

/* it appears that the mc1332x radio cannot */
/* receive packets where the last three bits of the first byte */
/* is equal to 2 --- even in promiscuous mode */
int maca_read(void *buf, unsigned short bufsize) {
	uint32_t i;
	volatile uint32_t rx_size;
	rx_size = reg32(MACA_GETRXLVL) - 4;
	if(rx_size < bufsize) bufsize = rx_size;
#if MACA_RAW_MODE
	PRINTF("maca read: in raw mode bufsize 0x%0x \n\r",bufsize);
	PRINTF("maca read:   \n\r");
	for(i=2; i<=bufsize; i++) {
	((uint8_t *)buf)[i-2] = rx_buf[i];
#else
	PRINTF("maca read: bufsize 0x%0x \n\r",bufsize);
	PRINTF("maca read:   \n\r");
	for(i=1; i<=bufsize; i++) {
	((uint8_t *)buf)[i-1] = rx_buf[i];
#endif
		PRINTF(" %02x",rx_buf[i]);
	}
	PRINTF("\n\r");
	return bufsize;
}

int maca_send(const void *payload, unsigned short payload_len) {
	volatile uint32_t i,j;
	volatile uint32_t retry;
	volatile uint32_t len;

	len = payload_len;

	/* wait for maca to finish what it's doing */
	while(status_is_not_completed());
	set_bit(reg32(GPIO_DATA0),8);
	ResumeMACASync();

//	for(retry=0; retry<4; retry++) {

/* the mc1322x promiscuous mode doen't appear to be entirely promiscuous */
/* in MACA_RAW_MODE, all transmitted packets are prepended with MACA_RAW_PREPEND */
/* received packets get stripped of this */
/* i.e. it's "raw" with respect to the upper layers of RIME */
#if MACA_RAW_MODE
	PRINTF("maca: in raw mode sending 0x%0x + %d bytes\n\r", MACA_RAW_PREPEND, payload_len);
	tx_buf[0] = MACA_RAW_PREPEND;
	len++;
	for(i=1; i<=payload_len; i++) {
#else
	PRINTF("maca: sending %d bytes\n\r", payload_len);
	for(i=0; i<payload_len; i++) {
#endif
		/* copy payload into tx buf */
		tx_buf[i] = ((uint8_t *)payload)[i-1];
		PRINTF(" %02x",((uint8_t *)payload)[i-1]);
//		PRINTF(" %02x",tx_buf[i]);
	}
	PRINTF("\n\r");


	/* set dma tx pointer to the payload */
	/* and set the tx len */
	reg32(MACA_TXLEN) = (uint32_t)(len+4);
	reg32(MACA_DMATX) = (uint32_t)tx_buf;
	reg32(MACA_DMARX) = (uint32_t)rx_buf;
	reg32(MACA_TMREN) = 0;
	/* do the transmit */
	reg32(MACA_CONTROL) = ( (1<<maca_ctrl_prm) |
				(maca_ctrl_mode_no_cca<<maca_ctrl_mode) |
				(1<<maca_ctrl_asap) |
				(maca_ctrl_seq_tx));	

	/* wait for transmit to finish */
	/* maybe wait until action complete instead? */
	while(!action_complete_irq());
	ResumeMACASync();
//	}
	clear_bit(reg32(GPIO_DATA0),8);
	return 0;
}

void maca_set_receiver(void (* recv)(const struct radio_driver *))
{
  receiver_callback = recv;
}


PROCESS(maca_process, "maca process");
PROCESS_THREAD(maca_process, ev, data)
{
	volatile uint32_t i;
	volatile uint16_t status;

	PROCESS_BEGIN();

	reg32(MACA_CONTROL) = ((1<<maca_ctrl_prm) | (1<<maca_ctrl_nofc) | (maca_ctrl_mode_no_cca<<maca_ctrl_mode));
	for(i=0; i<400000; i++) { continue; }

	ResumeMACASync();

	while(1) {		

#ifndef DISABLE_RECEPTION
		/* if we aren't doing anything */
		/* should also check that there is nothing that has been recieved */
		if(!(status_is_not_completed()) && 
		   !(data_indication_irq())
			) 
		{
			/* start a reception */
//			PRINTF("maca: starting reception sequence\n\r");
			ResumeMACASync();
			/* this sets the rxlen field */
			/* this is undocumented but very important */
			/* you will not receive anything without setting it */
			reg32(MACA_TXLEN) = (MAX_PACKET_SIZE << 16);
			reg32(MACA_DMATX) = (uint32_t)&tx_buf;
			reg32(MACA_DMARX) = (uint32_t)&rx_buf;
			/* with timeout */		    
			reg32(MACA_SFTCLK) = reg32(MACA_CLK) + MACA_SOFT_TIMEOUT;
			reg32(MACA_TMREN) = ((1<<maca_tmren_cpl) | (1<<maca_tmren_sft));
			/* start the receive sequence */ 
			reg32(MACA_CONTROL) = ( (1<<maca_ctrl_prm) | 
						(1<<maca_ctrl_asap) |
						(maca_ctrl_seq_rx));
		}

		if(data_indication_irq()) {
			/* call the recieve callback */
			/* then do something? */
			set_bit(reg32(GPIO_DATA0),9);
			
			receiver_callback(&maca_driver);
			
			reg32(MACA_CLRIRQ) = (1<<maca_irq_di);			

			clear_bit(reg32(GPIO_DATA0),9);
		}
#endif /* DISABLE_RECEPTION */

		if(action_complete_irq()) {
			reg32(MACA_CLRIRQ) = (1<<maca_irq_acpl);
			status = reg32(MACA_STATUS) & 0x0000ffff;
			switch(status)
			{
			case(maca_cc_aborted):
			{
				PRINTF("maca: aborted\n\r");
				ResumeMACASync();				
				break;
				
			}
			case(maca_cc_not_completed):
			{
//				PRINTF("maca: not completed\n\r");
				ResumeMACASync();
				break;
				
			}
			case(maca_cc_timeout):
			{
				PRINTF("maca: timeout\n\r");
				ResumeMACASync();
				break;
				
			}
			case(maca_cc_no_ack):
			{
				PRINTF("maca: no ack\n\r");
				ResumeMACASync();
				break;
				
			}
			case(maca_cc_ext_timeout):
			{
//				PRINTF("maca: ext timeout\n\r");
				ResumeMACASync();
				break;
				
			}
			case(maca_cc_ext_pnd_timeout):
			{
				PRINTF("maca: ext pnd timeout\n\r");
				ResumeMACASync();
				break;
			}
			case(maca_cc_success):
			{
				PRINTF("maca: success\n\r");
				break;				
			}
			default:
			{
				PRINTF("status: %x",status);
				ResumeMACASync();
				
			}
			}
		} else if (filter_failed_irq()) {
			PRINTF("filter failed\n\r");
			ResumeMACASync();
			reg32(MACA_CLRIRQ) = (1<<maca_irq_flt);
		} else if (checksum_failed_irq()) {
			PRINTF("checksum failed\n\r");
			ResumeMACASync();
			reg32(MACA_CLRIRQ) = (1<<maca_irq_crc);
		}
		
		PROCESS_PAUSE();
		
	};
	
	PROCESS_END();
}



/* internal mc1322x routines */

#define MACA_CLOCK_DIV 95

static uint8_t ram_values[4];

void init_phy(void)
{
  volatile uint32_t cnt;

  reg32(MACA_RESET) = (1<<maca_reset_rst);
 
  for(cnt=0; cnt < 100; cnt++); 

  reg32(MACA_RESET) = (1<<maca_reset_clkon);

  reg32(MACA_CONTROL) = maca_ctrl_seq_nop;

  for(cnt=0; cnt < 400000; cnt++); 

  reg32(MACA_TMREN) = (1<<maca_tmren_strt) | (1<<maca_tmren_cpl);
  reg32(MACA_CLKDIV) = MACA_CLOCK_DIV;
  reg32(MACA_WARMUP) = 0x00180012;    
  reg32(MACA_EOFDELAY) = 0x00000004;  
  reg32(MACA_CCADELAY) = 0x001a0022;   
  reg32(MACA_TXCCADELAY) = 0x00000025;
  reg32(MACA_FRAMESYNC0) = 0x000000A7; 
  reg32(MACA_CLK) = 0x00000008;       
  reg32(MACA_MASKIRQ) = ((1<<maca_irq_rst) | (1<<maca_irq_acpl) | (1<<maca_irq_cm) | (1<<maca_irq_flt) | (1<<maca_irq_crc) | (1<<maca_irq_di));
  reg32(MACA_SLOTOFFSET) = 0x00350000; 


}

void reset_maca(void)
{
	uint32_t tmp;
	reg32(MACA_CONTROL) = maca_ctrl_seq_nop;
	do
	{
		tmp = reg32(MACA_STATUS);
	}
	while ((tmp & 0xf) == maca_cc_not_completed);
	
	/* Clear all interrupts. */
	reg32(MACA_CLRIRQ) = 0xffff;
}

#define RF_BASE 0x80009a00
void flyback_init(void) {
	uint32_t val8, or;
	
	val8 = reg32(RF_BASE+8);
	or = val8 | 0x0000f7df;
	reg32(RF_BASE+8) = or;
	reg32(RF_BASE+12) = 0x00ffffff;
	reg32(RF_BASE+16) = (((uint32_t)0x00ffffff)>>12);
	reg32(RF_BASE) = 16;
	/* good luck and godspeed */
}

/* all of this calibration data was pulled from the ROM  */
/* should probably do that instead of hardcoding it here */
/* also should probably run it through the execute_entry */

#define MAX_SEQ1 2
const uint32_t addr_seq1[MAX_SEQ1] = {
	0x80003048,      
	0x8000304c,
};

const uint32_t data_seq1[MAX_SEQ1] = {
	0x00000f78,     
	0x00607707,
};


#define MAX_SEQ2 2
const uint32_t addr_seq2[MAX_SEQ2] = {
	0x8000a050,      
	0x8000a054,      
};

const uint32_t data_seq2[MAX_SEQ2] = {
	0x0000047b,
	0x0000007b, 
};

#define MAX_CAL3_SEQ1 3
const uint32_t addr_cal3_seq1[MAX_CAL3_SEQ1] = { 0x80009400,0x80009a04,0x80009a00, };
const uint32_t data_cal3_seq1[MAX_CAL3_SEQ1] = {0x00020017,0x8185a0a4,0x8c900025, };

#define MAX_CAL3_SEQ2 2
const uint32_t addr_cal3_seq2[MAX_CAL3_SEQ2] = { 0x80009a00,0x80009a00,};
const uint32_t data_cal3_seq2[MAX_CAL3_SEQ2] = { 0x8c900021,0x8c900027,};

#define MAX_CAL3_SEQ3 1
const uint32_t addr_cal3_seq3[MAX_CAL3_SEQ3] = { 0x80009a00 };
const uint32_t data_cal3_seq3[MAX_CAL3_SEQ3] = { 0x8c900000 };

#define MAX_CAL5 4
const uint32_t addr_cal5[MAX_CAL5] = { 
	0x80009400,  
	0x8000a050,       
	0x8000a054,  
	0x80003048,
};
const uint32_t data_cal5[MAX_CAL5] = {
	0x00000017,
	0x00000000,            
	0x00000000,
	0x00000f00,
};

#define MAX_DATA 43
const uint32_t addr_reg_rep[MAX_DATA] = { 0x80004118,0x80009204,0x80009208,0x8000920c,0x80009210,0x80009300,0x80009304,0x80009308,0x8000930c,0x80009310,0x80009314,0x80009318,0x80009380,0x80009384,0x80009388,0x8000938c,0x80009390,0x80009394,0x8000a008,0x8000a018,0x8000a01c,0x80009424,0x80009434,0x80009438,0x8000943c,0x80009440,0x80009444,0x80009448,0x8000944c,0x80009450,0x80009460,0x80009464,0x8000947c,0x800094e0,0x800094e4,0x800094e8,0x800094ec,0x800094f0,0x800094f4,0x800094f8,0x80009470,0x8000981c,0x80009828 };

const uint32_t data_reg_rep[MAX_DATA] = { 0x00180012,0x00000605,0x00000504,0x00001111,0x0fc40000,0x20046000,0x4005580c,0x40075801,0x4005d801,0x5a45d800,0x4a45d800,0x40044000,0x00106000,0x00083806,0x00093807,0x0009b804,0x000db800,0x00093802,0x00000015,0x00000002,0x0000000f,0x0000aaa0,0x01002020,0x016800fe,0x8e578248,0x000000dd,0x00000946,0x0000035a,0x00100010,0x00000515,0x00397feb,0x00180358,0x00000455,0x00000001,0x00020003,0x00040014,0x00240034,0x00440144,0x02440344,0x04440544,0x0ee7fc00,0x00000082,0x0000002a };


/* has been tested and it good */
void vreg_init(void) {
	volatile uint32_t i;
	reg32(0x80003000) = 0x00000018; /* set default state */
	reg32(0x80003048) = 0x00000f04; /* bypass the buck */
	for(i=0; i<0x161a8; i++) { continue; } /* wait for the bypass to take */
//	while((((*(volatile uint32_t *)(0x80003018))>>17) & 1) !=1) { continue; } /* wait for the bypass to take */
	reg32(0x80003048) = 0x00000ff8; /* start the regulators */
}

/* get_ctov thanks to Umberto */

#define _INIT_CTOV_WORD_1       0x00dfbe77
#define _INIT_CTOV_WORD_2       0x023126e9
uint8_t get_ctov( uint32_t r0, uint32_t r1 )
{

        r0 = r0 * _INIT_CTOV_WORD_1;
        r0 += ( r1 << 22 );
        r0 += _INIT_CTOV_WORD_2;

        r0 = (uint32_t)(((int32_t)r0) >> 25);

        return (uint8_t)r0;
}

/* initialized with 0x4c */
uint8_t ctov[16] = {
	0x0b,
	0x0b,
	0x0b,
	0x0a,
	0x0d,
	0x0d,
	0x0c,
	0x0c,
	0x0f,
	0x0e,
	0x0e,
	0x0e,
	0x11,
	0x10,
	0x10,
	0x0f,
};


/* radio_init has been tested to be good */
void radio_init(void) {
	volatile uint32_t i;
	/* sequence 1 */
	for(i=0; i<MAX_SEQ1; i++) {
		reg32(addr_seq1[i]) = data_seq1[i];
	}
	/* seq 1 delay */
	for(i=0; i<0x161a8; i++) { continue; }
	/* sequence 2 */
	for(i=0; i<MAX_SEQ2; i++) {
		reg32(addr_seq2[i]) = data_seq2[i];
	}
	/* modem val */
	reg32(0x80009000) = 0x80050100;
	/* cal 3 seq 1*/
	for(i=0; i<MAX_CAL3_SEQ1; i++) {
		reg32(addr_cal3_seq1[i]) = data_cal3_seq1[i];
	}
	/* cal 3 delay */
	for(i=0; i<0x11194; i++) { continue; }
	/* cal 3 seq 2*/
	for(i=0; i<MAX_CAL3_SEQ2; i++) {
		reg32(addr_cal3_seq2[i]) = data_cal3_seq2[i];
	}
	/* cal 3 delay */
	for(i=0; i<0x11194; i++) { continue; }
	/* cal 3 seq 3*/
	for(i=0; i<MAX_CAL3_SEQ3; i++) {
		reg32(addr_cal3_seq3[i]) = data_cal3_seq3[i];
	}
	/* cal 5 */
	for(i=0; i<MAX_CAL5; i++) {
		reg32(addr_cal5[i]) = data_cal5[i];
	}
	/*reg replacment */
	for(i=0; i<MAX_DATA; i++) {
		reg32(addr_reg_rep[i]) = data_reg_rep[i];
	}
	
	reg32(0x80003048) = 0x00000f04; /* bypass the buck */
	for(i=0; i<0x161a8; i++) { continue; } /* wait for the bypass to take */
//	while((((*(volatile uint32_t *)(0x80003018))>>17) & 1) !=1) { continue; } /* wait for the bypass to take */
	reg32(0x80003048) = 0x00000fa4; /* start the regulators */
	for(i=0; i<0x161a8; i++) { continue; } /* wait for the bypass to take */

	init_from_flash(0x1F000);

	PRINTF("radio_init: ctov parameter 0x%x\n\r",ram_values[3]);
	for(i=0; i<16; i++) {
		ctov[i] = get_ctov(i,ram_values[3]);
		PRINTF("radio_init: ctov[%d] = 0x%x\n\r",i,ctov[i]);
	}

}

const uint32_t PSMVAL[19] = {
	0x0000080f,
	0x0000080f,
	0x0000080f,
	0x0000080f,
	0x0000081f,
	0x0000081f,
	0x0000081f,
	0x0000080f,
	0x0000080f,
	0x0000080f,
	0x0000001f,
	0x0000000f,
	0x0000000f,
	0x00000816,
	0x0000001b,
	0x0000000b,
	0x00000802,
	0x00000817,
	0x00000003,
};

const uint32_t PAVAL[19] = {
	0x000022c0,
	0x000022c0,
	0x000022c0,
	0x00002280,
	0x00002303,
	0x000023c0,
	0x00002880,
	0x000029f0,
	0x000029f0,
	0x000029f0,
	0x000029c0,
	0x00002bf0,
	0x000029f0,
	0x000028a0,
	0x00002800,
	0x00002ac0,
	0x00002880,
	0x00002a00,
	0x00002b00,
};

const uint32_t AIMVAL[19] = {
	0x000123a0,
	0x000163a0,
	0x0001a3a0,
	0x0001e3a0,
	0x000223a0,
	0x000263a0,
	0x0002a3a0,
	0x0002e3a0,
	0x000323a0,
	0x000363a0,
	0x0003a3a0,
	0x0003a3a0,
	0x0003e3a0,
	0x000423a0,
	0x000523a0,
	0x000423a0,
	0x0004e3a0,
	0x0004e3a0,
	0x0004e3a0,
};

/* tested and seems to be good */
#define ADDR_POW1 0x8000a014
#define ADDR_POW2 ADDR_POW1 + 12
#define ADDR_POW3 ADDR_POW1 + 64
void set_power(uint8_t power) {
	reg32(ADDR_POW1) = PSMVAL[power];
	reg32(ADDR_POW2) = (ADDR_POW1>>18) | PAVAL[power];
	reg32(ADDR_POW3) = AIMVAL[power];
}

const uint8_t VCODivI[16] = {
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x30,
	0x30,
	0x30,
	0x2f,
	0x30,
	0x30,
	0x30,
	0x30,
};

const uint32_t VCODivF[16] = {
	0x00355555,
	0x006aaaaa,
	0x00a00000,
	0x00d55555,
	0x010aaaaa,
	0x01400000,
	0x01755555,
	0x01aaaaaa,
	0x01e00000,
	0x00155555,
	0x004aaaaa,
	0x00800000,
	0x00b55555,
	0x00eaaaaa,
	0x01200000,
	0x01555555,		
};

/* tested good */
#define ADDR_CHAN1 0x80009800
#define ADDR_CHAN2 (ADDR_CHAN1+12)
#define ADDR_CHAN3 (ADDR_CHAN1+16)
#define ADDR_CHAN4 (ADDR_CHAN1+48)
void set_channel(uint8_t chan) {
	volatile uint32_t tmp;

	tmp = reg32(ADDR_CHAN1);
	tmp = tmp & 0xbfffffff;
	reg32(ADDR_CHAN1) = tmp;

	reg32(ADDR_CHAN2) = VCODivI[chan];
	reg32(ADDR_CHAN3) = VCODivF[chan];

	tmp = reg32(ADDR_CHAN4);
	tmp = tmp | 2;
	reg32(ADDR_CHAN4) = tmp;

	tmp = reg32(ADDR_CHAN4);
	tmp = tmp | 4;
	reg32(ADDR_CHAN4) = tmp;

	tmp = tmp & 0xffffe0ff;
	tmp = tmp | (((ctov[chan])<<8)&0x1F00);
	reg32(ADDR_CHAN4) = tmp;
	/* duh! */
}

#define ROM_END 0x0013ffff
#define ENTRY_EOF 0x00000e0f
/* processes up to 4 words of initialization entries */
/* returns the number of words processed */
uint32_t exec_init_entry(volatile uint32_t *entries, uint8_t *valbuf) 
{
	volatile uint32_t i;
	if(entries[0] <= ROM_END) {
		if (entries[0] == 0) {
			/* do delay command*/
			for(i=0; i<entries[1]; i++) { continue; }
			return 2;
		} else if (entries[0] == 1) {
			/* do bit set/clear command*/
			reg32(entries[2]) = (reg32(entries[2]) & ~entries[1]) | (entries[3] & entries[1]);
			return 4;
		} else if ((entries[0] >= 16) &&
			   (entries[0] < 0xfff1)) {
			/* store bytes in valbuf */
			valbuf[(entries[0]>>4)-1] = entries[1];
			return 2;
		} else if (entries[0] == ENTRY_EOF) {
//			puts("init_entry: eof ");
			return 0;
		} else {
			/* invalid command code */
			return 0;
		}
	} else { /* address isn't in ROM space */   
		 /* do store value in address command  */
		reg32(entries[0]) = entries[1];
		return 2;
	}
}


#define FLASH_INIT_MAGIC 0x00000abc
uint32_t init_from_flash(uint32_t addr) {
	nvm_type_t type=0;
	nvm_err_t err;	
	volatile uint32_t buf[8];
	volatile uint16_t len;
	volatile uint32_t i=0,j;
	err = nvm_detect(NVM_INTERFACE_INTERNAL, &type);
		
	nvm_setsvar(0);
	err = nvm_read(NVM_INTERFACE_INTERNAL, type, (uint8_t *)buf, addr, 8);
	i+=8;
	
	if(buf[0] == FLASH_INIT_MAGIC) {
		len = buf[1] & 0x0000ffff;
		while(i<len-4) {
			volatile uint32_t ret;
			err = nvm_read(NVM_INTERFACE_INTERNAL, type, (uint8_t *)buf, addr+i, 32);
			i += 4*exec_init_entry(buf, ram_values);
		}
		return i;
	} else {
		return 0;
	}
 	
}

/* 
 * Do the ABORT-Wait-NOP-Wait sequence in order to prevent MACA malfunctioning.
 * This seqeunce is synchronous and no interrupts should be triggered when it is done.
 */
void ResumeMACASync(void)
{ 
  uint32_t clk, TsmRxSteps, LastWarmupStep, LastWarmupData, LastWarmdownStep, LastWarmdownData;
//  bool_t tmpIsrStatus;
  volatile uint32_t i;

//  ITC_DisableInterrupt(gMacaInt_c);  
//  AppInterrupts_ProtectFromMACAIrq(tmpIsrStatus); <- Original from MAC code, but not sure how is it implemented

  /* Manual TSM modem shutdown */

  /* read TSM_RX_STEPS */
  TsmRxSteps = (*((volatile uint32_t *)(0x80009204)));
 
  /* isolate the RX_WU_STEPS */
  /* shift left to align with 32-bit addressing */
  LastWarmupStep = (TsmRxSteps & 0x1f) << 2;
  /* Read "current" TSM step and save this value for later */
  LastWarmupData = (*((volatile uint32_t *)(0x80009300 + LastWarmupStep)));

  /* isolate the RX_WD_STEPS */
  /* right-shift bits down to bit 0 position */
  /* left-shift to align with 32-bit addressing */
  LastWarmdownStep = ((TsmRxSteps & 0x1f00) >> 8) << 2;
  /* write "last warmdown data" to current TSM step to shutdown rx */
  LastWarmdownData = (*((volatile uint32_t *)(0x80009300 + LastWarmdownStep)));
  (*((volatile uint32_t *)(0x80009300 + LastWarmupStep))) = LastWarmdownData;

  /* Abort */
  reg32(MACA_CONTROL) = maca_ctrl_seq_abort;
  
  /* Wait ~8us */
  for (clk = reg32(MACA_CLK), i = 0; reg32(MACA_CLK) - clk < 3 && i < 300; i++);
 
  /* NOP */
  reg32(MACA_CONTROL) = maca_ctrl_seq_nop;
  
  /* Wait ~8us */  
  for (clk = reg32(MACA_CLK), i = 0; reg32(MACA_CLK) - clk < 3 && i < 300; i++);
   

  /* restore original "last warmup step" data to TSM (VERY IMPORTANT!!!) */
  (*((volatile uint32_t *)(0x80009300 + LastWarmupStep))) = LastWarmupData;

  /* Clear all MACA interrupts - we should have gotten the ABORT IRQ */
  reg32(MACA_CLRIRQ) = 0xffff;

  /* need to */
  /* renable interrupts if they were enabled */
}
#if 0
#endif
