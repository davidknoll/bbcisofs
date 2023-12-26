#ifndef _SWROM_H_
#define _SWROM_H_

/* Sideways ROM entry points */
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

extern volatile unsigned char workspace_is_mine;
extern void *get_private(void);

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
extern void outhl(unsigned long l);
#ifdef DEBUG
extern void hexdump(const void *ptr, unsigned int len);
#endif /* DEBUG */
extern void outstr(const unsigned char *str);
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

struct dec_datetime {
    unsigned char year[4];
    unsigned char month[2];
    unsigned char day[2];
    unsigned char hour[2];
    unsigned char minute[2];
    unsigned char second[2];
    unsigned char csec[2];
    unsigned char tzoffset;
};

struct isopvd {
    unsigned char type;
    unsigned char standard[5];
    unsigned char version;
    unsigned char _unused1;
    unsigned char system[32];
    unsigned char volume[32];
    unsigned char _unused2[8];
    unsigned long vol_sz_l;
    unsigned long vol_sz_m;
    unsigned char _unused3[32];
    unsigned int volset_sz_l;
    unsigned int volset_sz_m;
    unsigned int volseq_l;
    unsigned int volseq_m;
    unsigned int blk_sz_l;
    unsigned int blk_sz_m;
    unsigned long ptbl_sz_l;
    unsigned long ptbl_sz_m;
    unsigned long ptbl_lba_l;
    unsigned long optbl_lba_l;
    unsigned long ptbl_lba_m;
    unsigned long optbl_lba_m;
    unsigned char rootdir[34];
    unsigned char volset[128];
    unsigned char publisher[128];
    unsigned char preparer[128];
    unsigned char application[128];
    unsigned char copyright[37];
    unsigned char abstract[37];
    unsigned char biblio[37];
    struct dec_datetime t_creation;
    struct dec_datetime t_modification;
    struct dec_datetime t_expiration;
    struct dec_datetime t_effective;
    unsigned char fstructver;
    unsigned char _unused4;
    unsigned char app_use[512];
    unsigned char _reserved[653];
};

extern unsigned char current_drive;
extern unsigned char current_directory[33 + 31];

extern struct isodirent *findfirst(struct isodirent *dirent, const unsigned char *name);
extern struct isodirent *loadrootdir(void);
extern unsigned char *loadtitle(void);
#ifdef DEBUG
extern void dirtest(void);
#endif /* DEBUG */

extern unsigned char *cachesector(unsigned char device, unsigned long lba);

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

struct osgbpb_control {
    unsigned char handle;
    unsigned long address;
    unsigned long count;
    unsigned long pointer;
};

#endif /* _SWROM_H_ */
