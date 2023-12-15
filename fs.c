#include <6502.h>
#include <string.h>
#include "swrom.h"

unsigned long osfile_handler(unsigned long axy) { return axy; }
unsigned long osargs_handler(unsigned long axy) { return axy; }
unsigned long osbget_handler(unsigned long axy) { return axy; }
unsigned long osbput_handler(unsigned long axy) { return axy; }
unsigned long osgbpb_handler(unsigned long axy) { return axy; }
unsigned long osfind_handler(unsigned long axy) { return axy; }
unsigned long osfsc_handler(unsigned long axy) { return axy; }

void fs_install(void)
{
    unsigned char rom = *((unsigned char *) 0xF4);
    void (**vectors)(void) = (void (**)(void)) 0x0200;
    struct extended_vector *extvectors;
    struct regs regs;
    memset(&regs, 0, sizeof regs);

    // Locate the extended vector table
    regs.a = 0xA8;
    regs.pc = OSBYTE;
    _sys(&regs);
    extvectors = (struct extended_vector *) ((regs.y << 8) | regs.x);

    // Call existing FSCV function 0x06 before taking over FS vectors
    regs.a = 0x06;
    regs.pc = (unsigned int) vectors[15];
    _sys(&regs);

    SEI();

    extvectors[ 9].vector = osfile_entry;
    extvectors[10].vector = osargs_entry;
    extvectors[11].vector = osbget_entry;
    extvectors[12].vector = osbput_entry;
    extvectors[13].vector = osgbpb_entry;
    extvectors[14].vector = osfind_entry;
    extvectors[15].vector = osfsc_entry;

    extvectors[ 9].rom = rom;
    extvectors[10].rom = rom;
    extvectors[11].rom = rom;
    extvectors[12].rom = rom;
    extvectors[13].rom = rom;
    extvectors[14].rom = rom;
    extvectors[15].rom = rom;

    vectors[ 9] = (void (*)(void)) (0xFF00 + (3 *  9)); // FILEV
    vectors[10] = (void (*)(void)) (0xFF00 + (3 * 10)); // ARGSV
    vectors[11] = (void (*)(void)) (0xFF00 + (3 * 11)); // BGETV
    vectors[12] = (void (*)(void)) (0xFF00 + (3 * 12)); // BPUTV
    vectors[13] = (void (*)(void)) (0xFF00 + (3 * 13)); // GBPBV
    vectors[14] = (void (*)(void)) (0xFF00 + (3 * 14)); // FINDV
    vectors[15] = (void (*)(void)) (0xFF00 + (3 * 15)); // FSCV

    CLI();

    // Call service ROM function 0x0F after taking over FS vectors
    regs.a = 0x8F;
    regs.x = 0x0F;
    regs.pc = OSBYTE;
    _sys(&regs);
}
