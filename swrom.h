#ifndef _SWROM_H_
#define _SWROM_H_

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

struct extended_vector {
    void (*vector)(void);
    unsigned char rom;
};

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
extern void idereset(void);

#define SECTOR_SIZE 2048

/* For debugging */
extern void outhn(unsigned char n);
extern void outhb(unsigned char b);
extern void outhw(unsigned int w);
extern void hexdump(const void *ptr, unsigned int len);
extern void outstr(const char *str);
extern void brk_error(unsigned char num, const unsigned char *msg);

/* OSWORD layer */
extern int handle_osword(unsigned char func, void *params);

struct osword_control {
    unsigned char controller;
    unsigned long address;
    unsigned char cmd_opcode;
    unsigned char cmd_lba[3];
    unsigned char cmd_count;
    unsigned char cmd_unused;
    unsigned long length;
};

struct osword_error {
    unsigned char lba[3];
    unsigned char error;
    unsigned char channel;
};

/* Directory manipulation */
struct isodirent {
    unsigned char direntlen;
    unsigned char xalen;
    unsigned long lba_l;
    unsigned long lba_m;
    unsigned long size_l;
    unsigned long size_m;
    unsigned char t_years;
    unsigned char t_month;
    unsigned char t_day;
    unsigned char t_hour;
    unsigned char t_minute;
    unsigned char t_second;
    unsigned char t_offset;
    unsigned char flags;
    unsigned char il_unitsz;
    unsigned char il_gapsz;
    unsigned int volseq_l;
    unsigned int volseq_m;
    unsigned char namelen;
    unsigned char name[];
};

extern unsigned char current_drive;
extern unsigned char current_directory[33 + 31];

extern struct isodirent *findfirst(struct isodirent *dirent, const unsigned char *name);
extern struct isodirent *loadrootdir(void);
extern void dirtest(void);

/* Filing system vectors */
extern void osfile_entry(void);
extern void osargs_entry(void);
extern void osbget_entry(void);
extern void osbput_entry(void);
extern void osgbpb_entry(void);
extern void osfind_entry(void);
extern void osfsc_entry(void);
extern unsigned long osfile_handler(unsigned long axy);
extern unsigned long osargs_handler(unsigned long axy);
extern unsigned long osbget_handler(unsigned long axy);
extern unsigned long osbput_handler(unsigned long axy);
extern unsigned long osgbpb_handler(unsigned long axy);
extern unsigned long osfind_handler(unsigned long axy);
extern unsigned long osfsc_handler(unsigned long axy);
extern void fs_install(void);

#define FS_NO 37

struct osfile_control {
    unsigned char *filename;
    unsigned long load;
    unsigned long execution;
    unsigned long length;
    unsigned long attributes;
};

#endif /* _SWROM_H_ */
