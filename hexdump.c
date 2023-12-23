#include "swrom.h"

static void __fastcall__ (*oswrch)(unsigned char c) = (void *) OSWRCH;
static void __fastcall__ (*osnewl)(void) = (void *) OSNEWL;

void outhn(unsigned char n)
{
  n &= 0x0F;
  if (n < 0x0A) {
    oswrch(n + '0');
  } else {
    oswrch(n + 'A' - 0x0A);
  }
}

void outhb(unsigned char b)
{
  outhn(b >> 4);
  outhn(b);
}

void outhw(unsigned int w)
{
  outhb(w >> 8);
  outhb(w);
}

void outhl(unsigned long l)
{
  outhw(l >> 8);
  outhw(l);
}

#ifdef DEBUG
static void dumprow(const unsigned char *ptr)
{
  unsigned char c, i;

  oswrch(' ');
  outhw((unsigned int) ptr);
  oswrch(' ');

  for (i = 0; i < 16; i++) {
    if (!(i % 8)) { oswrch(' '); }
    outhb(ptr[i]);
    oswrch(' ');
  }

  for (i = 0; i < 16; i++) {
    if (!(i % 8)) { oswrch(' '); }
    c = ptr[i];
    if (c >= 0x20 && c <= 0x7E) {
      oswrch(c);
    } else {
      oswrch('.');
    }
  }

  osnewl();
}

void hexdump(const void *ptr, unsigned int len)
{
  const unsigned char *startptr = (void *) ((unsigned int) ptr & 0xFFF0);
  const unsigned char *endptr = (void *) (((unsigned int) ptr + len - 1) | 0x000F);

  while (startptr < endptr) {
    dumprow(startptr);
    startptr += 16;
  }
}
#endif /* DEBUG */

// Output a string, accounting for UNIX line endings
void outstr(const unsigned char *str)
{
    while (*str) {
        if (*str == '\n') {
            osnewl();
        } else {
            oswrch(*str);
        }
        str++;
    }
}
