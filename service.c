#include <6502.h>
#include <ctype.h>
#include <string.h>
#include "swrom.h"

static void __fastcall__ (*oswrch)(unsigned char c) = (void *) OSWRCH;
static void __fastcall__ (*osnewl)(void) = (void *) OSNEWL;

// Output a string, accounting for UNIX line endings
static void outstr(const char *str)
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

// Perform a case-insensitive comparison of a word on the command line
static unsigned char cmdmatch(const struct regs *regs, const char *cmd)
{
    const char *cmdline = *((char **) 0xF2);
    unsigned char y = regs->y, i = 0;

    while (1) {
        if (isgraph(cmdline[y]) && isgraph(cmd[i])) {
            // Both are printable characters
            if (toupper(cmdline[y]) == toupper(cmd[i])) {
                // Both are the same character, case-insensitively
                y++;
                i++;
            } else {
                // Both are different characters, strings don't match
                return 0;
            }
        } else if (!isgraph(cmdline[y]) && !isgraph(cmd[i])) {
            // Both strings ended at the same length, if we get here they match
            return 1;
        } else {
            // Only one string has ended, different lengths, no match
            return 0;
        }
    }
}

// Eject/load the disc
static void eject(void)
{
    unsigned char data[3];
    mode_sense(1, 0, sizeof data, data);
    if (data[2] == 0x71) {
        // Door is open, close it
        start_stop_unit(1, 1, 3);
    } else {
        // Door is closed, open it
        start_stop_unit(1, 1, 2);
    }
}

// Test reading the CD using our OSWORD function
static void cdtest3(void)
{
    unsigned char oswdata[0xF];
    struct regs oswregs;
    memset(oswdata, 0, sizeof oswdata);
    memset(&oswregs, 0, sizeof oswregs);
    *((unsigned long *) (oswdata + 0x1)) = 0xFFFF4000;
    *((unsigned long *) (oswdata + 0xB)) = 2 * SECTOR_SIZE;
    oswdata[0x5] = 0x08;
    oswdata[0x6] = 0x20;
    oswdata[0x8] = 0x10;
    oswdata[0x9] = 2;
    oswregs.a = 0x26;
    oswregs.x = ((unsigned int) oswdata);
    oswregs.y = ((unsigned int) oswdata) >> 8;
    oswregs.pc = OSWORD;
    _sys(&oswregs);
    hexdump((void *) 0x4000, 2 * SECTOR_SIZE);
}

// Service ROM entry point (instead of main)
void __fastcall__ service(struct regs *regs)
{
    switch (regs->a) {
    case 0x04: // *command
        if (cmdmatch(regs, "EJECT")) {
            eject();
            regs->a = 0;
        } else if (cmdmatch(regs, "CDTEST")) {
            inquiry(1, (void *) 0x4000);
            hexdump((void *) 0x4000, 36);
            read_capacity(1, (void *) 0x4000);
            hexdump((void *) 0x4000, 4);
            read_sectors(1, 16, 1, (void *) 0x4000);
            hexdump((void *) 0x4000, SECTOR_SIZE);
            regs->a = 0;
        } else if (cmdmatch(regs, "CDTEST2")) {
            request_sense(1, (void *) 0x4000);
            mode_sense(1, 0x2A, 32, (void *) 0x4000);
            hexdump((void *) 0x4000, 32);
            regs->a = 0;
        } else if (cmdmatch(regs, "CDTEST3")) {
            cdtest3();
            regs->a = 0;
        }
        break;

    case 0x08: // OSWORD
        if (handle_osword(*((unsigned char *) 0xEF), *((void **) 0xF0))) {
            regs->a = 0;
        }
        break;

    case 0x09: // *HELP
        if (cmdmatch(regs, "CDFS")) {
            outstr("\nCD-ROM Filing System\n  EJECT\n  CDTEST\n  CDTEST2\n  CDTEST3\n");
            regs->a = 0;
        } else if (cmdmatch(regs, "")) {
            outstr("\nCD-ROM Filing System\n  CDFS\n");
        }
        break;
    }
}
