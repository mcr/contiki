#include <stdio.h>
#include <string.h>

#include "uart1.h"
#include "contiki-conf.h"

#undef putchar
#undef putc
#undef puts

int
putchar(int c)
{
  dbg_putchar(c);
  return c;
}

int
putc(int c, FILE *f)
{
  dbg_putchar(c);
  return c;
}

int
puts(const char *s)
{
	unsigned int i=0;
	while(s && *s!=0) {
		putchar(*s++); i++;
	}
	return i;
}

