#include <6502.h>
#include <string.h>
#include "swrom.h"

static unsigned char __fastcall__ (*osrdch)(void) = (void *) OSRDCH;
static void __fastcall__ (*oswrch)(unsigned char c) = (void *) OSWRCH;

static int loadfileattr(unsigned long axy)
{
    struct osword_control oswdata;
    struct osword_error oswerr;
    struct regs oswregs;
    struct isodirent *dirent;
    struct osfile_control *params = (struct osfile_control *) (axy >> 8);

    if (current_drive > 3) { return -1; }    // Error: invalid drive
    dirent = loadrootdir();
    if (dirent == 0) { return -2; }          // Error: couldn't load root dir entry / PVD
    dirent = findfirst(dirent, params->filename);
    if (dirent == 0) { return -3; }          // Error: couldn't find/load requested dir entry

    // Until we add the loading of Beeb metadata from .INF files
    if ((axy & 0xFF) == 0x05) {
        params->load = 0xFFFF0E00;
        params->execution = 0xFFFF802B;
        params->length = dirent->size_l;
        params->attributes = 0x0000005D;
    }

    if (dirent->flags & 0x02) { return -4; } // Error: is a directory
    if ((axy & 0xFF) == 0x05) { return 0; }

    // Load data into memory based on LBA and size from the directory entry
    // Default load address, until we can read the .INF files
    oswdata.controller = current_drive >> 1;
    oswdata.address = (params->execution & 0xFF) ? 0xFFFF0E00 : params->load;
    oswdata.cmd_opcode = 0x08; // READ(6)
    oswdata.cmd_lba[0] = ((current_drive << 5) & 0x20) | ((dirent->lba_l >> 16) & 0x1F);
    oswdata.cmd_lba[1] = dirent->lba_l >> 8;
    oswdata.cmd_lba[2] = dirent->lba_l;
    oswdata.cmd_count = 0; // Use the size in bytes
    oswdata.cmd_unused = 0;
    oswdata.length = dirent->size_l;

    oswregs.a = 0x26; // Access the disc controller
    oswregs.x = ((unsigned int) &oswdata);
    oswregs.y = ((unsigned int) &oswdata) >> 8;
    oswregs.flags = 0;
    oswregs.pc = OSWORD;
    _sys(&oswregs);

    // Request sense on error, mostly to clear the flag
    if (oswdata.controller > 0) {
        oswregs.a = 0x27; // Read last error information
        oswregs.x = ((unsigned int) &oswerr);
        oswregs.y = ((unsigned int) &oswerr) >> 8;
        _sys(&oswregs);
    }
    return oswdata.controller;
}

unsigned long osfile_handler(unsigned long axy)
{
    struct osfile_control *params = (struct osfile_control *) (axy >> 8);

    switch (axy & 0xFF) {
    case 0xFF: // Load file
        axy &= ~0xFF;
        switch (loadfileattr(axy)) {
        case 0:
            axy |= 0x01;
            break;
        case -1:
            brk_error(0xCD, "Invalid drive");
            break;
        case -2:
            brk_error(0xD3, "Bad or no disc");
            break;
        case -3:
            brk_error(0xD6, "Not found");
            break;
        case -4:
            brk_error(0xB5, "Not a file");
            break;
        default:
            brk_error(0xC7, "Drive error");
            break;
        }
        break;

    case 0x05: // Read load, exec, attributes
        axy &= ~0xFF;
        switch (loadfileattr(axy)) {
        case 0:
            axy |= 0x01;
            break;
        case -1:
            brk_error(0xCD, "Invalid drive");
            break;
        case -2:
            brk_error(0xD3, "Bad or no disc");
            break;
        case -3:
            // axy |= 0x00;
            break;
        case -4:
            axy |= 0x02;
            break;
        default:
            brk_error(0xC7, "Drive error");
            break;
        }
        break;

    case 0x00: // Save file
    case 0x01: // Write load, exec, attributes
    case 0x02: // Write load address
    case 0x03: // Write exec address
    case 0x04: // Write attributes
    case 0x06: // Delete
    case 0x07: // Create file
    case 0x08: // Create directory
        brk_error(0xC9, "Read only");
        break;
    }
    return axy;
}

unsigned long osargs_handler(unsigned long axy)
{
    unsigned long *params = (unsigned long *) ((axy >> 8) & 0x00FF);
    if (axy & 0x00FF0000) { brk_error(0xDE, "Not open"); }

    switch (axy & 0xFF) {
    case 0x00: // Return filing system number
        axy &= ~0xFF;
        axy |= FS_NO;
        break;

    case 0x01: // Return address of command line tail
        axy &= ~0xFF;
        *params = 0xFFFF0000 | *((unsigned int *) 0xF2);
        break;
    }
    return axy;
}

unsigned long osbget_handler(unsigned long axy)
{
    if (axy & 0x00FF0000) { brk_error(0xDE, "Not open"); }
    axy &= ~0xFF;
    axy |= osrdch();
    return axy;
}

unsigned long osbput_handler(unsigned long axy)
{
    if (axy & 0x00FF0000) { brk_error(0xDE, "Not open"); }
    oswrch(axy & 0xFF);
    return axy;
}

unsigned long osgbpb_handler(unsigned long axy)
{
    struct osgbpb_control *params = (struct osgbpb_control *) (axy >> 8);
    unsigned char *title;
    unsigned char *titlelen = (unsigned char *) params->address;

    switch (axy & 0xFF) {
    case 0x01: // Write bytes to file at specified pointer
    case 0x02: // Write bytes to file at current pointer
        if (params->handle) { brk_error(0xDE, "Not open"); }
        axy &= ~0xFF;
        while (params->count--) {
            oswrch(*((unsigned char *) (params->address++)));
            params->pointer++;
        }
        break;

    case 0x03: // Read bytes from file at specified pointer
    case 0x04: // Read bytes from file at current pointer
        if (params->handle) { brk_error(0xDE, "Not open"); }
        axy &= ~0xFF;
        while (params->count--) {
            *((unsigned char *) (params->address++)) = osrdch();
            params->pointer++;
        }
        break;

    case 0x05: // Read title and option from current drive
        title = loadtitle();
        if (title == 0) { brk_error(0xD3, "Bad or no disc"); }
        axy &= ~0xFF;

        // Length of title
        if (params->count == 0) { break; }
        *titlelen = 0;
        params->address++;
        params->count--;

        // Title
        while (*titlelen < 32 && title[*titlelen] > ' ') {
            if (params->count) {
                *((unsigned char *) (params->address++)) = title[*titlelen];
                params->count--;
            }
            (*titlelen)++;
        }

        // Boot option, drive number
        if (params->count == 0) { break; }
        *((unsigned char *) (params->address++)) = 0;
        params->count--;
        if (params->count == 0) { break; }
        *((unsigned char *) (params->address++)) = current_drive;
        params->count--;
        break;

    case 0x06: // Read current drive and directory names
    case 0x07: // Read library drive and directory names
        axy &= ~0xFF;

        // Drive number, as a string
        if (params->count == 0) { break; }
        *((unsigned char *) (params->address++)) = 1;
        params->count--;
        if (params->count == 0) { break; }
        *((unsigned char *) (params->address++)) = '0' + current_drive;
        params->count--;

        // Directory name, as a string
        if (params->count == 0) { break; }
        *((unsigned char *) (params->address++)) = 1;
        params->count--;
        if (params->count == 0) { break; }
        *((unsigned char *) (params->address++)) = '$';
        params->count--;
        break;

    case 0x08: // Read file names from the current directory
        axy &= ~0xFF;
        break;
    }
    return axy;
}

unsigned long osfind_handler(unsigned long axy)
{
    switch (axy & 0xFF) {
    case 0x00: // Close file
        if (axy & 0x00FF0000) { brk_error(0xDE, "Not open"); }
        break;
    case 0x40: // Open for input
        axy &= ~0xFF;
        break;
    case 0x80: // Open for output
    case 0xC0: // Open for update
        brk_error(0xC9, "Read only");
        break;
    }
    return axy;
}

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
