#include <6502.h>
#include <ctype.h>
#include "swrom.h"

static unsigned char dirsecbuf[SECTOR_SIZE];
unsigned char current_drive = 1;
unsigned char current_directory[33 + 31];
// unsigned char library_drive = 1;
// unsigned char library_directory[33 + 31];
// isodirent is 33 bytes upto/inc name length byte
// name length can be up to 1Fh or 31 chars if compliant, 222 chars if not
// (222 chars, being even, would attract a padding field)
// this length includes NAME.EXT;1 and does not include the length or padding fields if any
// after the padding field if any immediately begins the SUSP/RR field if any

// Check whether the given directory entry's name field matches the given name
static unsigned char matchdirent(struct isodirent *dirent, const unsigned char *name)
{
    unsigned char idx = 0;
    unsigned char namechar;

    // Special case: . (PC/Linux) or @ (BBC) or \0 (ISO)
    if (
        dirent->namelen == 0x01 && dirent->name[0] == 0x00 &&
        name[0] == '@' && name[1] <= ' '
    ) { return 1; }
    // Special case: .. (PC/Linux) or ^ (BBC) or \1 (ISO)
    if (
        dirent->namelen == 0x01 && dirent->name[0] == 0x01 &&
        name[0] == '^' && name[1] <= ' '
    ) { return 1; }

    // Don't care about the version number suffix
    while (idx < dirent->namelen && dirent->name[idx] != ';') {
        namechar = name[idx];
        // We have a character here from the directory entry, but no more
        // characters in the provided name, therefore the names are different
        // lengths, therefore they don't match
        if (namechar <= ' ') { return 0; }

        // Translate certain characters between BBC and non-BBC filenames
        switch (namechar) {
        case '/': namechar = '.'; break;
        case '<': namechar = '$'; break;
        case '>': namechar = '^'; break;
        case '?': namechar = '#'; break;
        }

        // We have a character from each name, but they don't match
        if (toupper(namechar) != toupper(dirent->name[idx])) { return 0; }
        // The names match up to this point
        idx++;
    }

    // Successful match; all chars match and same length
    if (name[idx] <= ' ') { return 1; }
    // There are more chars left in provided name
    return 0;
}

// Load the sector with the given LBA from the current drive into the directory sector buffer
static unsigned char loaddirsec(unsigned long lba)
{
    struct osword_control oswdata;
    struct osword_error oswerr;
    struct regs oswregs;
    if (current_drive > 3) { return -1; }

    // Construct a command packet
    oswdata.controller = current_drive >> 1;
    oswdata.address = 0xFFFF0000 | (unsigned int) dirsecbuf;
    oswdata.cmd_opcode = 0x08; // READ(6)
    oswdata.cmd_lba[0] = ((current_drive << 5) & 0x20) | ((lba >> 16) & 0x1F);
    oswdata.cmd_lba[1] = lba >> 8;
    oswdata.cmd_lba[2] = lba;
    oswdata.cmd_count = 1;
    oswdata.cmd_unused = 0;
    oswdata.length = sizeof dirsecbuf;

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

// Find a directory entry bearing the given name, within the loaded directory sector
static struct isodirent *finddirent(const unsigned char *name)
{
    struct isodirent *curdirent = (struct isodirent *) dirsecbuf;
    while (curdirent < (struct isodirent *) (dirsecbuf + sizeof dirsecbuf)) {
        // End of list; not found at least in this sector
        if (curdirent->direntlen == 0) { break; }
        // Success
        if (matchdirent(curdirent, name)) { return curdirent; }
        // Point to the next directory entry
        curdirent = (struct isodirent *) (((unsigned char *) curdirent) + curdirent->direntlen);
    }
    return 0;
}

// Load and search through each sector in turn,
// of the directory described by the given directory entry on the current drive,
// for the first directory entry bearing the given name
struct isodirent *findfirst(struct isodirent *dirent, const unsigned char *name)
{
    unsigned long lba = dirent->lba_l;
    unsigned long size = dirent->size_l;
    struct isodirent *result;

    while (size > 0) {
        // Try 3 times to load the directory sector
        if (loaddirsec(lba) != 0) {
            if (loaddirsec(lba) != 0) {
                if (loaddirsec(lba) != 0) {
                    return 0;
                }
            }
        }

        // Scan it for a matching entry
        result = finddirent(name);
        if (result != 0) { return result; }

        // Next sector?
        lba++;
        if (size < SECTOR_SIZE) {
            size = 0;
        } else {
            size -= SECTOR_SIZE;
        }
    }
    return 0;
}

// Load the PVD into the directory sector buffer
// and return a pointer to the root directory entry, or null on error
struct isodirent *loadrootdir(void)
{
    // Try 3 times to load the PVD
    if (loaddirsec(16) != 0) {
        if (loaddirsec(16) != 0) {
            if (loaddirsec(16) != 0) {
                return 0;
            }
        }
    }
    return (struct isodirent *) (dirsecbuf + 156);
}

// Load the PVD into the directory sector buffer
// and return a pointer to the volume identifier, or null on error
unsigned char *loadtitle(void)
{
    // Try 3 times to load the PVD
    if (loaddirsec(16) != 0) {
        if (loaddirsec(16) != 0) {
            if (loaddirsec(16) != 0) {
                return 0;
            }
        }
    }
    return dirsecbuf + 40;
}

#ifdef DEBUG
// Test loading a directory and picking a name out of it
void dirtest(void)
{
    struct isodirent *rootent, *foundent;

    // Read PVD
    unsigned char rtn = loaddirsec(16);
    outstr("read pvd rtn ");
    outhb(rtn);
    outstr("\n");

    // Dump root dir entry
    rootent = (struct isodirent *) (dirsecbuf + 156);
    outstr("root ent ");
    outhw((unsigned int) rootent);
    outstr("\n");
    hexdump(rootent, rootent->direntlen);

    // Dump found dir entry
    foundent = findfirst(rootent, "VBoxLinuxAdditions.run");
    outstr("found ent ");
    outhw((unsigned int) foundent);
    outstr("\n");
    hexdump(foundent, foundent->direntlen);
}
#endif /* DEBUG */
