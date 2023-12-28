#include <6502.h>
#include <ctype.h>
#include <string.h>
#include "swrom.h"

// Details of the shared absolute workspace segment
extern void _SHARED_RUN__;
extern void _SHARED_SIZE__;

// FS information for the Master
static const unsigned char fsinfo[11] = {
    'C', 'D', 'F', 'S', ' ', ' ', ' ', ' ',
    FH_MIN, FH_MAX, FS_NO
};

// Perform a case-insensitive comparison of a word on the command line
static unsigned char cmdmatch(const struct regs *regs, const unsigned char *cmd)
{
    const unsigned char *cmdline = *((char **) 0xF2);
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
    unsigned char device = get_private()->current_drive;
    unsigned char data[3];
    mode_sense(device, 0, sizeof data, data);
    if (data[2] == 0x71) {
        // Door is open, close it
        start_stop_unit(device, 1, 3);
    } else {
        // Door is closed, open it
        start_stop_unit(device, 1, 2);
    }
}

#ifdef DEBUG
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
#endif /* DEBUG */

// Get the address of our private workspace
struct private_workspace *get_private(void)
{
    return (void *) (((unsigned char *) 0x0DF0)[*((unsigned char *) 0xF4)] << 8);
}

// Service ROM entry point (instead of main)
void __fastcall__ service(struct regs *regs)
{
    unsigned int tmp;

    switch (regs->a) {
    case 0x01: // Request absolute workspace
        tmp = (unsigned int) &_SHARED_RUN__ + (unsigned int) &_SHARED_SIZE__;
        if (tmp & 0xFF) { tmp += 0x100; }
        if (regs->y < (tmp >> 8)) { regs->y = (tmp >> 8); }
        break;

    case 0x02: // Request private workspace
        tmp = sizeof (struct private_workspace);
        if (tmp & 0xFF) { tmp += 0x100; }
        ((unsigned char *) 0x0DF0)[regs->x] = regs->y;
        regs->y += (tmp >> 8);

        tmp = (unsigned int) get_private();
        memset((struct private_workspace *) tmp, 0, sizeof (struct private_workspace));
        ((struct private_workspace *) tmp)->error_drive = 0xFF;
        ((struct private_workspace *) tmp)->secbufdev = 0xFF;
        ((struct private_workspace *) tmp)->current_drive = 1;
        break;

    case 0x04: // *command
        if (cmdmatch(regs, "CDFS")) {
            fs_install();
            regs->a = 0;
        } else if (cmdmatch(regs, "EJECT")) {
            eject();
            regs->a = 0;
        } else if (cmdmatch(regs, "IDERESET")) {
            idereset();
            regs->a = 0;
#ifdef DEBUG
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
        } else if (cmdmatch(regs, "DIRTEST")) {
            dirtest();
            regs->a = 0;
#endif /* DEBUG */
        }
        break;

    case 0x08: // OSWORD
        if (handle_osword(*((unsigned char *) 0xEF), *((void **) 0xF0))) {
            regs->a = 0;
        }
        break;

    case 0x09: // *HELP
        if (cmdmatch(regs, "CDFS")) {
            outstr("\nCD-ROM Filing System\n");
            outstr("  CDFS\n  EJECT\n  IDERESET\n");
#ifdef DEBUG
            outstr("  CDTEST\n  CDTEST2\n  CDTEST3\n  DIRTEST\n");
#endif /* DEBUG */
        } else if (cmdmatch(regs, "")) {
            outstr("\nCD-ROM Filing System\n");
            outstr("  CDFS\n");
        }
        break;

    case 0x0A: // Absolute workspace is being claimed
        get_private()->workspace_is_mine = 0;
        break;

    case 0x12: // Select filing system
        if (regs->y == FS_NO) {
            fs_install();
            regs->a = 0;
        }
        break;

    case 0x25: // Return filing system information (Master)
        memcpy(*((unsigned char **) 0xF2) + regs->y, fsinfo, sizeof fsinfo);
        regs->y += sizeof fsinfo;
        break;
    }
}
