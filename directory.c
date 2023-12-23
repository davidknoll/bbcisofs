#include <ctype.h>
#include "swrom.h"

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

// Find a directory entry bearing the given name, within the loaded directory sector
static struct isodirent *finddirent(unsigned char *dirsecbuf, const unsigned char *name)
{
    struct isodirent *curdirent = (struct isodirent *) dirsecbuf;
    struct isodirent *enddirent = (struct isodirent *) (dirsecbuf + SECTOR_SIZE);

    while (curdirent < enddirent) {
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
    unsigned char *dirsecbuf;
    struct isodirent *result;

    while (size > 0) {
        dirsecbuf = cachesector(current_drive, lba);

        // Scan it for a matching entry
        result = finddirent(dirsecbuf, name);
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
    unsigned char *dirsecbuf = cachesector(current_drive, 16);
    if (!dirsecbuf) { return 0; }
    return (struct isodirent *) (dirsecbuf + 156);
}

// Load the PVD into the directory sector buffer
// and return a pointer to the volume identifier, or null on error
unsigned char *loadtitle(void)
{
    unsigned char *dirsecbuf = cachesector(current_drive, 16);
    unsigned char unpad = 32;
    if (!dirsecbuf) { return 0; }

    dirsecbuf += 40;
    while (unpad--) {
        if (dirsecbuf[unpad] == ' ') {
            dirsecbuf[unpad] = '\0';
        } else {
            break;
        }
    }
    return dirsecbuf;
}

#ifdef DEBUG
// Test loading a directory and picking a name out of it
void dirtest(void)
{
    struct isodirent *rootent, *foundent;

    // Dump root dir entry
    rootent = loadrootdir();
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
