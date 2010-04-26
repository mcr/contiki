#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* contiki */
#include "radio.h"
#include "sys/process.h"

#include "mc1322x.h"

#define CONTIKI_MACA_DEBUG 1
#if CONTIKI_MACA_DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifndef MACA_RAW_PREPEND
#define MACA_RAW_PREPEND 0xff
#endif

/* contiki mac driver */

static void (* receiver_callback)(const struct radio_driver *);

int maca_on_request(void);
int maca_off_request(void);
int maca_read(void *buf, unsigned short bufsize);
int maca_send(const void *data, unsigned short len);
void maca_set_receiver(void (* recv)(const struct radio_driver *d));

const struct radio_driver maca_driver =
{
	.send = maca_send,
	.read = maca_read,
	.on = maca_on_request,
	.off = maca_off_request,
};

static volatile uint8_t maca_request_on = 0;
static volatile uint8_t maca_request_off = 0;

static process_event_t event_data_ready;

int maca_on_request(void) {
	maca_request_on = 1;
	maca_request_off = 0;
	return 1;
}

int maca_off_request(void) {
	maca_request_on = 0;
	maca_request_off = 1;
	return 1;
}

/* it appears that the mc1332x radio cannot */
/* receive packets where the last three bits of the first byte */
/* is equal to 2 --- even in promiscuous mode */
int maca_read(void *buf, unsigned short bufsize) {
	volatile uint32_t i;
	volatile packet_t *p;

	if((p = rx_packet())) {
		PRINTF("maca read");
#if MACA_RAW_MODE
		/* offset + 1 and size - 1 to strip the raw mode prepended byte */
		/* work around since maca can't receive acks bigger than five bytes */
		PRINTF(" raw mode");
		p->length -= 1;
		p->offset += 1;
#endif
		if((p->length) < bufsize) bufsize = (p->length);
		memcpy(buf, (uint8_t *)(p->data + p->offset), bufsize);
		PRINTF(": bufsize 0x%0x \n\r",bufsize);
		PRINTF("maca read:   \n\r");
#if CONTIKI_MACA_DEBUG
		for( i = p->offset ; i < (bufsize + p->offset) ; i++) {
			PRINTF(" %02x",p->data[i]);
		}
#endif 
		PRINTF("\n\r");
		free_packet(p);
		return bufsize;
	} else {
		return 0;
	}
}

int maca_send(const void *payload, unsigned short payload_len) {
	volatile int i;
	volatile packet_t *p;

	if ((p = get_free_packet())) {
		PRINTF("maca send");
		maca_on();
#if MACA_RAW_MODE
		p->offset = 1;
		p->length = payload_len + 1;
#else 
		p->offset = 0;
		p->length = payload_len;
#endif
		if(payload_len > MAX_PACKET_SIZE)  return RADIO_TX_ERR;
		memcpy((uint8_t *)(p->data + p->offset), payload, payload_len);
#if MACA_RAW_MODE
		p->offset = 0;
		p->data[0] = MACA_PREPEND_BYTE;
		PRINTF(" raw mode");
#endif
#if CONTIKI_MACA_DEBUG
		PRINTF(": sending %d bytes\n\r", payload_len);
		for(i = p->offset ; i < (p->length + p->offset); i++) {
			PRINTF(" %02x",p->data[i]);
		}
		PRINTF("\n\r");
#endif
		tx_packet(p);
		return RADIO_TX_OK;
	} else {
		PRINTF("couldn't get free packet for maca_send\n\r");
		return RADIO_TX_ERR;
	}
}

void maca_set_receiver(void (* recv)(const struct radio_driver *))
{
  receiver_callback = recv;
}

PROCESS(maca_process, "maca process");
PROCESS_THREAD(maca_process, ev, data)
{
 	volatile uint32_t i;
	
 	PROCESS_BEGIN();

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == event_data_ready);

		/* check if there is a request to turn the radio on or off */
		if(maca_request_on == 1) {
			maca_request_on = 0;
			maca_on();
 		}

		if(maca_request_off == 1) {
			maca_request_off = 0;
			maca_off();
 		}
		
		if (rx_head != NULL) {
			receiver_callback(&maca_driver);
		}
		
 	};
	
 	PROCESS_END();
}

void maca_rx_callback(volatile packet_t *p __attribute((unused))) {
	process_post(&maca_process, event_data_ready, NULL);
}
