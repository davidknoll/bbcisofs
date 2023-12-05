#include <string.h>
#include "swrom.h"

int packet_cmd(unsigned char device, unsigned char *packet, unsigned char *data, unsigned long datalen)
{
    // Select device and wait for it to be ready
    struct ide_interface *ide = (device & 2)
        ? (struct ide_interface *) IDE_SECONDARY
        : (struct ide_interface *) IDE_PRIMARY;
    while ((ide->command & 0xC0) != 0x40);  // BSY not set, DRDY set
    ide->head = (device & 1) ? 0xB0 : 0xA0; // Master or slave
    while ((ide->command & 0xC0) != 0x40);

    // Issue the PACKET command
    ide->error = 0x00;
    ide->cylinder_lo = 0xFF;
    ide->cylinder_hi = 0xFF;
    ide->command = 0xA0;

    while (ide->command & 0x89) {                       // BSY, DRQ or CHK
        if (ide->command & 0x80) { continue; }          // If BSY, wait
        if (ide->command & 0x01) { return ide->error; } // If CHK, return error

        if (ide->command & 0x08) {
            switch (ide->count & 0x03) {
            case 0:
                // Output data to device
                // If end of buffer, write zeroes
                if (datalen > 1) {
                    ide->data_hi = *(data + 1);
                    ide->data_lo = *data;
                    data += 2;
                    datalen -= 2;
                } else {
                    ide->data_hi = 0;
                    if (datalen > 0) {
                        ide->data_lo = *data;
                        datalen--;
                    } else {
                        ide->data_lo = 0;
                    }
                }
                break;

            case 1:
                // Output command packet to device
                ide->data_hi = *(packet + 1);
                ide->data_lo = *packet;
                packet += 2;
                break;

            case 2:
                // Input data from device
                // If end of buffer, discard surplus
                if (datalen > 0) {
                    *data++ = ide->data_lo;
                    datalen--;
                    if (datalen > 0) {
                        *data++ = ide->data_hi;
                        datalen--;
                    }
                } else {
                    ide->data_lo;
                }
                break;

            case 3:
                // Input command packet from device (shouldn't happen?)
                *packet++ = ide->data_lo;
                *packet++ = ide->data_hi;
                break;
            }
        }
    }

    return 0;
}

int inquiry(unsigned char device, unsigned char *data)
{
    unsigned char packet[12];
    memset(packet, 0, sizeof packet);
    packet[0] = 0x12;
    packet[4] = 36;
    return packet_cmd(device, packet, data, 36);
}

int request_sense(unsigned char device, unsigned char *data)
{
    unsigned char packet[12];
    memset(packet, 0, sizeof packet);
    packet[0] = 0x03;
    packet[4] = 18;
    return packet_cmd(device, packet, data, 18);
}

int start_stop_unit(unsigned char device, unsigned char immediate, unsigned char mode)
{
    unsigned char packet[12];
    memset(packet, 0, sizeof packet);
    packet[0] = 0x1B;
    packet[1] = immediate & 1;
    packet[4] = mode & 3;
    return packet_cmd(device, packet, 0, 0);
}

int test_unit_ready(unsigned char device)
{
    unsigned char packet[12];
    memset(packet, 0, sizeof packet);
    return packet_cmd(device, packet, 0, 0);
}

int read_capacity(unsigned char device, unsigned long *lba)
{
    unsigned char packet[12];
    unsigned char data[8];
    int rtn;
    memset(packet, 0, sizeof packet);
    packet[0] = 0x25;
    rtn = packet_cmd(device, packet, data, sizeof data);
    if (rtn) { return rtn; }
    *lba = ((unsigned long) data[0] << 24)
        | ((unsigned long) data[1] << 16)
        | ((unsigned long) data[2] << 8)
        | data[3];
    *lba++;
    return 0;
}

int read_sectors(unsigned char device, unsigned long lba, unsigned int count, unsigned char *data)
{
    unsigned char packet[12];
    memset(packet, 0, sizeof packet);
    packet[0] = 0x28;
    packet[2] = lba >> 24;
    packet[3] = lba >> 16;
    packet[4] = lba >> 8;
    packet[5] = lba;
    packet[7] = count >> 8;
    packet[8] = count;
    return packet_cmd(device, packet, data, count * SECTOR_SIZE);
}

int mode_sense(unsigned char device, unsigned char page, unsigned int alloc, unsigned char *data)
{
    unsigned char packet[12];
    memset(packet, 0, sizeof packet);
    packet[0] = 0x5A;
    packet[2] = page;
    packet[7] = alloc >> 8;
    packet[8] = alloc;
    return packet_cmd(device, packet, data, alloc);
}
