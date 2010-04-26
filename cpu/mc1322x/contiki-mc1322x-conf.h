#ifndef __CONTIKI_MC1322X_CONF_H__
#define __CONTIKI_MC1322X_CONF_H__

typedef int32_t s32_t;

/*
 * MCU and clock rate
 */
#define MCU_MHZ 24
#define PLATFORM PLATFORM_MC1322X

/* Pre-allocated memory for loadable modules heap space (in bytes)*/
#define MMEM_CONF_SIZE 256

#define AUTOSTART_ENABLE 1

#define CCIF
#define CLIF

typedef uint32_t clock_time_t;
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned long u32_t;
typedef unsigned short uip_stats_t;

void clock_delay(unsigned int us2);
void clock_wait(int ms10);
void clock_set_seconds(unsigned long s);
unsigned long clock_seconds(void);

#endif
