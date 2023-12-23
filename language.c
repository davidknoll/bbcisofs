#include <6502.h>
#include <string.h>
#include "swrom.h"

// Calls OSCLI
static void oscli(const unsigned char *cmd)
{
    struct regs osregs;
    memset(&osregs, 0, sizeof osregs);
    osregs.x = ((unsigned int) cmd);
    osregs.y = ((unsigned int) cmd) >> 8;
    osregs.pc = OSCLI;
    _sys(&osregs);
}

// Language ROM entry point (instead of main)
void __fastcall__ language(struct regs *regs)
{
    switch (regs->a) {
    case 0x01: // Enter language
        // I'm not a real language, use OSCLI to issue *BASIC
        oscli("BASIC\r");
        break;
    }
}
