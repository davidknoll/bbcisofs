#include <6502.h>
#include "swrom.h"

static unsigned char secbuf[SECTOR_SIZE];
static unsigned char secbufdev = 0xFF;
static unsigned long secbuflba;

// Load the sector with the given LBA from the given drive into the sector buffer
static unsigned char *cachesector_aux(unsigned char device, unsigned long lba)
{
    struct osword_control oswdata;
    struct osword_error oswerr;
    struct regs oswregs;
    if (device > 3) { return 0; }
    if (device == secbufdev && lba == secbuflba) { return secbuf; }

    // Construct a command packet
    oswdata.controller = device >> 1;
    oswdata.address = 0xFFFF0000 | (unsigned int) secbuf;
    oswdata.cmd_opcode = 0x08; // READ(6)
    oswdata.cmd_lba[0] = ((device << 5) & 0x20) | ((lba >> 16) & 0x1F);
    oswdata.cmd_lba[1] = lba >> 8;
    oswdata.cmd_lba[2] = lba;
    oswdata.cmd_count = 1;
    oswdata.cmd_unused = 0;
    oswdata.length = sizeof secbuf;

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
        secbufdev = 0xFF;
        return 0;
    }

    secbufdev = device;
    secbuflba = lba;
    return secbuf;
}

// Load the sector with the given LBA from the given drive into the sector buffer, with retries
unsigned char *cachesector(unsigned char device, unsigned long lba)
{
    unsigned char retries = 3;
    unsigned char *result;
    while (retries--) {
        result = cachesector_aux(device, lba);
        if (result != 0) { break; }
    }
    return result;
}
