/* Sideways ROM entry points */
extern void __fastcall__ language(struct regs *regs);
extern void __fastcall__ service(struct regs *regs);

/* MOS calls */
#define OSRDRM 0xFFB9
#define VDUCHR 0xFFBC
#define OSEVEN 0xFFBF
#define GSINIT 0xFFC2
#define GSREAD 0xFFC5
#define NVRDCH 0xFFC8
#define NVWRCH 0xFFCB
#define OSFIND 0xFFCE
#define OSGBPB 0xFFD1
#define OSBPUT 0xFFD4
#define OSBGET 0xFFD7
#define OSARGS 0xFFDA
#define OSFILE 0xFFDD
#define OSRDCH 0xFFE0
#define OSASCI 0xFFE3
#define OSNEWL 0xFFE7
#define OSWRCH 0xFFEE
#define OSWORD 0xFFF1
#define OSBYTE 0xFFF4
#define OSCLI  0xFFF7

/* I/O ports for J.G.Harston & Sprow's 16-bit IDE interface */
struct ide_interface {
  volatile unsigned char data_lo;
  volatile unsigned char error;
  volatile unsigned char count;
  volatile unsigned char sector;
  volatile unsigned char cylinder_lo;
  volatile unsigned char cylinder_hi;
  volatile unsigned char head;
  volatile unsigned char command;
  volatile unsigned char data_hi;
  volatile unsigned char _dummy[5];
  volatile unsigned char alt_status;
  volatile unsigned char dev_address;
};

#define IDE_PRIMARY 0xFC40
#define IDE_SECONDARY 0xFC50

/* ATAPI CD-ROM commands */
extern int packet_cmd(unsigned char device, unsigned char *packet, unsigned char *data, unsigned long datalen);
extern int inquiry(unsigned char device, unsigned char *data);
extern int request_sense(unsigned char device, unsigned char *data);
extern int start_stop_unit(unsigned char device, unsigned char immediate, unsigned char mode);
extern int test_unit_ready(unsigned char device);
extern int read_capacity(unsigned char device, unsigned long *lba);
extern int read_sectors(unsigned char device, unsigned long lba, unsigned int count, unsigned char *data);
extern int mode_sense(unsigned char device, unsigned char page, unsigned int alloc, unsigned char *data);

#define SECTOR_SIZE 2048

/* For debugging */
extern void hexdump(const void *ptr, unsigned int len);

extern int handle_osword(unsigned char func, void *params);
