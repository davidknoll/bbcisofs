#include <6502.h>
#include <string.h>
#include "swrom.h"
#define XVECENT(n) ((void (*)(void)) (0xFF00 + (3 * (n))))

// Load a file's metadata and/or data into memory
static int loadfileattr(unsigned long axy)
{
    struct osword_control oswdata;
    struct osword_error oswerr;
    struct regs oswregs;
    struct isodirent *dirent;
    struct osfile_control *params = (struct osfile_control *) (axy >> 8);
    unsigned char device = get_private()->current_drive;

    // Find the directory entry
    if (device > 3) { return -1; }           // Error: invalid drive
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
    oswdata.controller = device >> 1;
    oswdata.address = (params->execution & 0xFF) ? 0xFFFF0E00 : params->load;
    oswdata.cmd_opcode = 0x08; // READ(6)
    oswdata.cmd_lba[0] = ((device << 5) & 0x20) | ((dirent->lba_l >> 16) & 0x1F);
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
    axy |= _osrdch();
    return axy;
}

unsigned long osbput_handler(unsigned long axy)
{
    if (axy & 0x00FF0000) { brk_error(0xDE, "Not open"); }
    _oswrch(axy & 0xFF);
    return axy;
}

// OSGBPB function 5
static void readtitleoption(struct osgbpb_control *params)
{
    unsigned char *title = loadtitle();
    unsigned char titlelen = strlen(title);
    if (title == 0) { brk_error(0xD3, "Bad or no disc"); }

    // Will it fit in the space available?
    if (params->count < 3) {
        titlelen = 0;
    } else if (params->count - 3 < titlelen) {
        titlelen = params->count - 3;
    }

    if (params->count) {
        *((unsigned char *) params->address++) = titlelen;
        params->count--;
    }
    if (params->count) {
        memcpy((unsigned char *) params->address, title, titlelen);
        params->address += titlelen;
        params->count -= titlelen;
    }
    if (params->count) {
        *((unsigned char *) params->address++) = 0; // Boot option
        params->count--;
    }
    if (params->count) {
        *((unsigned char *) params->address++) = get_private()->current_drive;
        params->count--;
    }
}

// Part of OSGBPB function 8
static void appendname(struct osgbpb_control *params, struct isodirent *dirent)
{
    // TODO filename character translation
    unsigned char *oldaddr = (unsigned char *) params->address++;
    while (*oldaddr < dirent->namelen) {
        if (dirent->name[*oldaddr] == ';') { break; }
        *((unsigned char *) params->address++) = dirent->name[(*oldaddr)++];
    }
}

// OSGBPB function 8
static void readfilenames(struct osgbpb_control *params)
{
    unsigned int dirsec = params->pointer >> 16;
    unsigned int diroff = params->pointer;
    struct isodirent *parent = loadrootdir();
    struct isodirent *current;
    unsigned long lba = parent->lba_l;
    unsigned long size = parent->size_l;
    if (parent == 0) { brk_error(0xC7, "Drive error"); }

    while (params->count) {
        // We've come to the actual end of a directory sector
        if (diroff >= SECTOR_SIZE) {
            dirsec++;
            diroff = 0;
        }

        // End of the directory listing; there are no more names to read
        if (((dirsec * SECTOR_SIZE) + diroff) >= size) { break; }

        // Get the directory entry pointed to
        current = (struct isodirent *) cachesector(get_private()->current_drive, lba + dirsec);
        if (current == 0) { brk_error(0xC7, "Drive error"); }
        current = (struct isodirent *) (((unsigned char *) current) + diroff);

        // We've come to padding at the end of a directory sector,
        // where a directory entry cannot cross a sector boundary
        if (current->direntlen == 0) {
            dirsec++;
            diroff = 0;
            continue;
        }

        // Copy the filename, advance to next entry
        params->count--;
        appendname(params, current);
        diroff += current->direntlen;
    }

    params->handle = 0; // Cycle number, we don't really have that here
    params->pointer = (((unsigned long) dirsec) << 16) | diroff;
}

unsigned long osgbpb_handler(unsigned long axy)
{
    struct osgbpb_control *params = (struct osgbpb_control *) (axy >> 8);

    switch (axy & 0xFF) {
    case 0x01: // Write bytes to file at specified pointer
    case 0x02: // Write bytes to file at current pointer
        if (params->handle) { brk_error(0xDE, "Not open"); }
        axy &= ~0xFF;
        while (params->count) {
            _oswrch(*((unsigned char *) (params->address++)));
            params->pointer++;
            params->count--;
        }
        break;

    case 0x03: // Read bytes from file at specified pointer
    case 0x04: // Read bytes from file at current pointer
        if (params->handle) { brk_error(0xDE, "Not open"); }
        axy &= ~0xFF;
        while (params->count) {
            *((unsigned char *) (params->address++)) = _osrdch();
            params->pointer++;
            params->count--;
        }
        break;

    case 0x05: // Read title and option from current drive
        axy &= ~0xFF;
        readtitleoption(params);
        break;

    case 0x06: // Read current drive and directory names
    case 0x07: // Read library drive and directory names
        axy &= ~0xFF;

        // Drive number, as a string
        if (params->count) {
            *((unsigned char *) (params->address++)) = 1;
            params->count--;
        }
        if (params->count) {
            *((unsigned char *) (params->address++)) = '0' + get_private()->current_drive;
            params->count--;
        }

        // Directory name, as a string
        if (params->count) {
            *((unsigned char *) (params->address++)) = 1;
            params->count--;
        }
        if (params->count) {
            *((unsigned char *) (params->address++)) = '$';
            params->count--;
        }
        break;

    case 0x08: // Read file names from the current directory
        axy &= ~0xFF;
        readfilenames(params);
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

// Part of FSCV functions 5 & 9
static void catheader(void)
{
    struct osgbpb_control gbpb;
    unsigned char buf[40];
    unsigned char i;

    gbpb.address = 0xFFFF0000 | (unsigned int) buf;
    gbpb.count = sizeof buf;
    osgbpb_handler((((unsigned int) &gbpb) << 8) | 0x05);
    for (i = 0; i < buf[0]; i++) { _oswrch(buf[i + 1]); }
    outstr("\nDrive ");
    outhn(buf[buf[0] + 2]);
    for (i = 0; i < 13; i++) { _oswrch(' '); }
    outstr("Option ");
    outhn(buf[buf[0] + 1]);

    gbpb.address = 0xFFFF0000 | (unsigned int) buf;
    gbpb.count = sizeof buf;
    osgbpb_handler((((unsigned int) &gbpb) << 8) | 0x06);
    outstr("\nDir. :");
    for (i = 0; i < buf[0]; i++) { _oswrch(buf[i + 1]); }
    _oswrch('.');
    for (i = 0; i < buf[buf[0] + 1]; i++) { _oswrch(buf[buf[0] + 2 + i]); }
    for (i = 0; i < (13 - (buf[0] + buf[buf[0] + 1])); i++) { _oswrch(' '); }

    gbpb.address = 0xFFFF0000 | (unsigned int) buf;
    gbpb.count = sizeof buf;
    osgbpb_handler((((unsigned int) &gbpb) << 8) | 0x07);
    outstr("Lib. :");
    for (i = 0; i < buf[0]; i++) { _oswrch(buf[i + 1]); }
    _oswrch('.');
    for (i = 0; i < buf[buf[0] + 1]; i++) { _oswrch(buf[buf[0] + 2 + i]); }
    _osnewl();
    _osnewl();
}

// Part of FSCV function 5
static void catnames(void)
{
    struct osgbpb_control gbpb;
    unsigned char buf[40];
    unsigned char i;
    gbpb.handle = 0;
    gbpb.pointer = 0;

    do {
        gbpb.address = 0xFFFF0000 | (unsigned int) buf;
        gbpb.count = 1;
        osgbpb_handler((((unsigned int) &gbpb) << 8) | 0x08);
        if (gbpb.count == 0) {
            for (i = 0; i < 4; i++) { _oswrch(' '); }
            for (i = 0; i < buf[0]; i++) { _oswrch(buf[i + 1]); }
            _osnewl();
        }
    } while (gbpb.count == 0);
}

// Part of FSCV functions 9 & 10
static void infoname(unsigned char *name)
{
    struct osfile_control attrs;
    unsigned char i, ft;

    attrs.filename = name;
    ft = osfile_handler((((unsigned int) &attrs) << 8) | 0x05);
    if (ft == 0) { brk_error(0xD6, "Not found"); }
    for (i = 0; attrs.filename[i] > ' '; i++) { _oswrch(attrs.filename[i]); }
    for (; i < 32; i++) { _oswrch(' '); }

    // Is the permissions stuff even needed?
    _oswrch((ft == 2) ? 'D' : ' ');
    _oswrch((attrs.attributes & 0x08) ? 'L' : '-');
    _oswrch((attrs.attributes & 0x04) ? 'E' : '-');
    _oswrch((attrs.attributes & 0x02) ? 'W' : '-');
    _oswrch((attrs.attributes & 0x01) ? 'R' : '-');
    _oswrch('/');
    _oswrch((attrs.attributes & 0x80) ? 'l' : '-');
    _oswrch((attrs.attributes & 0x40) ? 'e' : '-');
    _oswrch((attrs.attributes & 0x20) ? 'w' : '-');
    _oswrch((attrs.attributes & 0x10) ? 'r' : '-');

    // And to add the start sector we'd need the actual directory entry
    _oswrch(' ');
    _oswrch(' ');
    outhl(attrs.load);
    _oswrch(' ');
    _oswrch(' ');
    outhl(attrs.execution);
    _oswrch(' ');
    _oswrch(' ');
    outhl(attrs.length);
    _osnewl();
}

// Part of FSCV function 9
static void exnames(void)
{
    struct osgbpb_control gbpb;
    unsigned char buf[40];
    gbpb.handle = 0;
    gbpb.pointer = 0;

    do {
        gbpb.address = 0xFFFF0000 | (unsigned int) buf;
        gbpb.count = 1;
        osgbpb_handler((((unsigned int) &gbpb) << 8) | 0x08);
        if (gbpb.count == 0) {
            buf[buf[0] + 1] = '\0';
            infoname(buf + 1);
        }
    } while (gbpb.count == 0);
}

// FSCV functions 2, 3, 4, 11
static void starrun(unsigned char *name)
{
    struct regs regs;
    struct osfile_control osf;
    osf.filename = name;
    osfile_handler((((unsigned int) &osf) << 8) | 0x05);

    // Find the command line tail
    while (*name == ' ') { name++; }
    while (*name > ' ') { name++; }
    while (*name == ' ') { name++; }
    *((unsigned char **) 0xF2) = name;

    if (osf.execution == 0xFFFFFFFF) {
        // *EXEC it
        brk_error(0xFF, "Unimplemented");
    } else if (osf.load == 0xFFFFFFFF) {
        brk_error(0x93, "Won't");
    } else {
        // Load and execute it
        memset(&regs, 0, sizeof regs);
        regs.pc = osf.execution;
        osf.execution = 0;
        osfile_handler((((unsigned int) &osf) << 8) | 0xFF);
        _sys(&regs);
    }
}

unsigned long osfsc_handler(unsigned long axy)
{
    unsigned char *params = (unsigned char *) (axy >> 8);

    switch (axy & 0xFF) {
    case 0x00: // *OPT
        if ((axy & 0x0000FF00) == 0x00000400) { brk_error(0xC9, "Read only"); }
        break;

    case 0x01: // EOF #
        axy &= ~0xFF;
        if (axy & 0x0000FF00) { brk_error(0xDE, "Not open"); }
        break;

    case 0x02: // */command
    case 0x03: // *command
    case 0x04: // *RUN command
    case 0x0B: // *RUN command from library
        axy &= ~0xFF;
        starrun(params);
        break;

    case 0x05: // *CAT
        // Only supports current directory
        axy &= ~0xFF;
        catheader();
        catnames();
        break;

    case 0x06: // New filing system about to take over
    case 0x08: // *command being processed
        // Do nothing, but return indicating function is supported
        axy &= ~0xFF;
        break;

    case 0x07: // File handle range
        axy = 0x005F5A00; // 5Ah to 5Fh
        break;

    case 0x09: // *EX
        // Only supports current directory
        axy &= ~0xFF;
        catheader();
        exnames();
        break;

    case 0x0A: // *INFO
        // Does not yet support wildcards
        axy &= ~0xFF;
        infoname(params);
        break;

    case 0x0C: // *RENAME
        axy &= ~0xFF;
        brk_error(0xC9, "Read only");
        break;
    }
    return axy;
}

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

    vectors[ 9] = XVECENT( 9); // FILEV -> XFILEV
    vectors[10] = XVECENT(10); // ARGSV -> XARGSV
    vectors[11] = XVECENT(11); // BGETV -> XBGETV
    vectors[12] = XVECENT(12); // BPUTV -> XBPUTV
    vectors[13] = XVECENT(13); // GBPBV -> XGBPBV
    vectors[14] = XVECENT(14); // FINDV -> XFINDV
    vectors[15] = XVECENT(15); // FSCV  -> XFSCV

    CLI();

    // Call service ROM function 0x0F after taking over FS vectors
    regs.a = 0x8F;
    regs.x = 0x0F;
    regs.pc = OSBYTE;
    _sys(&regs);

    // Call service ROM function 0x0A to claim the absolute workspace
    regs.a = 0x8F;
    regs.x = 0x0A;
    regs.pc = OSBYTE;
    _sys(&regs);

    get_private()->workspace_is_mine = 1;
}
