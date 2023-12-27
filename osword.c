#include <string.h>
#include "swrom.h"

static void osword_controller(unsigned char *params)
{
    unsigned char device = ((params[0] << 1) & 2) | ((params[6] >> 5) & 1);
    unsigned char packet[12];
    unsigned char *data = *((unsigned char **) (params + 1));
    unsigned long datalen;
    memset(packet, 0, sizeof packet);

    // If sector count is zero, interpret the length field
    if (params[9] > 0) {
        datalen = params[9] * SECTOR_SIZE;
    } else {
        datalen = *((unsigned long *) (params + 0xB));
        params[9] = datalen / SECTOR_SIZE;
        if (datalen % SECTOR_SIZE > 0) { params[9]++; }
    }

    // Some opcodes listed in AIV and ADFS user guides are not listed in SFF-8020
    // Passing a READ(6) straight through didn't work with BeebEm on VMware
    switch (params[5]) {
    case 0x01: // Translate REZERO UNIT to SEEK(10) to sector 0
        packet[0] = 0x2B;
        break;

    case 0x08: // Translate READ(6) to READ(10)
    case 0x0A: // Translate WRITE(6) to WRITE(10) (unlikely)
    case 0x0B: // Translate SEEK(6) to SEEK(10)
        packet[0] = params[5] | 0x20;
        packet[3] = params[6] & 0x1F;
        packet[4] = params[7];
        packet[5] = params[8];
        packet[8] = params[9];
        break;

    default:
        memcpy(packet, params + 5, 6);
        packet[1] &= 0x1F;
    }

    // Return sense key in control block, if any
    params[0] = packet_cmd(device, packet, data, datalen) >> 4;
    if (params[0] != 0) { get_private()->error_drive = device; }
}

static void osword_error(unsigned char *params)
{
    // The error code returned here will most likely just be 70h, as the output
    // of REQUEST SENSE has changed since the old SASI to ST-506 controllers
    unsigned char error_drive = get_private()->error_drive;
    unsigned char data[18];
    request_sense(error_drive, data);
    params[0] = data[6];
    params[1] = data[5];
    params[2] = (error_drive << 5) | (data[4] & 0x1F);
    params[3] = data[0];
    params[4] = 0;
}

int handle_osword(unsigned char func, void *params)
{
    switch (func) {
    case 0x24: // Read MSN and status byte
        // Temporary placeholder
        *((unsigned int *) params) = 0x2000;
        return 1;

    case 0x25: // Read free space on disc
        // There is no free space, it's a CD-ROM
        *((unsigned long *) params) = 0;
        return 1;

    case 0x26: // Access the disc controller
        osword_controller(params);
        return 1;

    case 0x27: // Read last error information
        osword_error(params);
        return 1;

    default:
        return 0;
    }
}
