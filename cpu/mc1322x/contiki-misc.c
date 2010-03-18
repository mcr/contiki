#include <stdio.h>
#include "contiki.h"
#include "dev/slip.h"


int raise(void)
{
	return 0;
}

void uip_log(char *msg)
{
  printf("uip: %s\n", msg);
}

static int
slip_putchar(char c, FILE *stream)
{
#define SLIP_END 0300
  static char debug_frame = 0;

  if (!debug_frame) {		/* Start of debug output */
    slip_arch_writeb(SLIP_END);
    slip_arch_writeb('\r'); /* Type debug line == '\r' */
    debug_frame = 1;
  }

  slip_arch_writeb((unsigned char)c);
  
  /*
   * Line buffered output, a newline marks the end of debug output and
   * implicitly flushes debug output.
   */
  if (c == '\n') {
    slip_arch_writeb(SLIP_END);
    debug_frame = 0;
  }

  return c;
}

//static FILE slip_stdout =
//FDEV_SETUP_STREAM(slip_putchar, NULL, _FDEV_SETUP_WRITE);

void
slip_arch_init(unsigned long ubr)
{
	// stdout = &slip_stdout;
}

void
slip_arch_writeb(unsigned char c)
{
	uart1_putc(c);
}

