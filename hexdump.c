#include <string.h>
#include "swrom.h"

void outhn(unsigned char n)
{
    n &= 0x0F;
    if (n < 0x0A) {
        _oswrch(n + '0');
    } else {
        _oswrch(n + 'A' - 0x0A);
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
    outhw(l >> 16);
    outhw(l);
}

// Output a string, accounting for UNIX line endings
void outstr(const unsigned char *str)
{
    while (*str) {
        if (*str == '\n') {
            _osnewl();
        } else {
            _oswrch(*str);
        }
        str++;
    }
}

// Assemble a BRK instruction with an error message in the stack space and execute it
void brk_error(unsigned char num, const unsigned char *msg)
{
    unsigned char *stk = (unsigned char *) 0x0100;
    stk[0] = 0x00;        // BRK opcode
    stk[1] = num;         // Error number
    strcpy(stk + 2, msg); // Error message, null-terminated
    ((void (*)(void)) stk)();
}

#ifdef DEBUG
static void dumprow(const unsigned char *ptr)
{
    unsigned char c, i;

    _oswrch(' ');
    outhw((unsigned int) ptr);
    _oswrch(' ');

    for (i = 0; i < 16; i++) {
        if (!(i % 8)) { _oswrch(' '); }
        outhb(ptr[i]);
        _oswrch(' ');
    }

    for (i = 0; i < 16; i++) {
        if (!(i % 8)) { _oswrch(' '); }
        c = ptr[i];
        if (c >= 0x20 && c <= 0x7E) {
        _oswrch(c);
        } else {
        _oswrch('.');
        }
    }

    _osnewl();
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
