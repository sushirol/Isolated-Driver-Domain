/* Copyright (C) 1992-2007 Univeristy of Minnesota, I/O Performance, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named 'Copying'; if not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139.
 */
/* Author:
 *      Tom Ruwart (tmruwart@ioperformance.com)
 *      I/O Perofrmance, Inc.
 */
/*
 * This file contains the subroutines necessary to support SCSI Generic I/O under LINUX.
 */
#if LINUX
#define _XOPEN_SOURCE 500
#include "xdd.h"


#define DEF_BLOCK_SIZE 512
#define DEF_BLOCKS_PER_TRANSFER 128

// #define SG_DEBUG

#define SENSE_BUFF_LEN 32       /* Arbitrary, could be larger */
#define READ_CAP_REPLY_LEN 8
#define DEF_TIMEOUT 40000       /* 40,000 millisecs == 40 seconds */

#ifndef RAW_MAJOR
#define RAW_MAJOR 255	/*unlikey value */
#endif 

#define FT_OTHER 0		/* filetype other than sg or raw device */
#define FT_SG 1			/* filetype is sg char device */
#define FT_RAW 2		/* filetype is raw char device */

static int sum_of_resids = 0;

static const char * proc_allow_dio = "/proc/scsi/sg/allow_dio";

int xdd_sg_filetype(const char * filename)
{
    struct stat st;

    if (stat(filename, &st) < 0)
	return FT_OTHER;
    if (S_ISCHR(st.st_mode)) {
	if (RAW_MAJOR == major(st.st_rdev))
	    return FT_RAW;
	else if (SCSI_GENERIC_MAJOR == major(st.st_rdev))
	    return FT_SG;
    }
    return FT_OTHER;
}

/* Return of 0 -> success, -1 -> failure, 2 -> try again */
int xdd_sg_read_capacity(int sg_fd, int * num_sect, int * sect_sz)
{
    int res;
    unsigned char rcCmdBlk [10] = {0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char rcBuff[READ_CAP_REPLY_LEN];
    unsigned char sense_b[64];
    sg_io_hdr_t io_hdr;

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = sizeof(rcCmdBlk);
    io_hdr.mx_sb_len = sizeof(sense_b);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = sizeof(rcBuff);
    io_hdr.dxferp = rcBuff;
    io_hdr.cmdp = rcCmdBlk;
    io_hdr.sbp = sense_b;
    io_hdr.timeout = DEF_TIMEOUT;

    if (ioctl(sg_fd, SG_IO, &io_hdr) < 0) {
        perror("read_capacity (SG_IO) error");
        return -1;
    }
    res = sg_err_category3(&io_hdr);
    if (SG_ERR_CAT_MEDIA_CHANGED == res)
        return 2; /* probably have another go ... */
    else if (SG_ERR_CAT_CLEAN != res) {
        fprintf(stderr,"read capacity error");
        return -1;
    }
    *num_sect = 1 + ((rcBuff[0] << 24) | (rcBuff[1] << 16) |
                (rcBuff[2] << 8) | rcBuff[3]);
    *sect_sz = (rcBuff[4] << 24) | (rcBuff[5] << 16) |
               (rcBuff[6] << 8) | rcBuff[7];
    return 0;
}

/* -1 -> unrecoverable error, 0 -> successful, 1 -> recoverable (ENOMEM),
   2 -> try again */
int xdd_sg_read(int sg_fd, unsigned char * buff, int blocks, int from_block, int bs, int * diop)
{
    unsigned char rdCmd[10] = {0x28, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char senseBuff[SENSE_BUFF_LEN];
    sg_io_hdr_t io_hdr;
    int res;

// fprintf(stderr,"xdd_sg_read: fd is %d, blocks is %d, from block is %d, bs is %d\n",sg_fd, blocks, from_block, bs);

    rdCmd[2] = (unsigned char)((from_block >> 24) & 0xFF);
    rdCmd[3] = (unsigned char)((from_block >> 16) & 0xFF);
    rdCmd[4] = (unsigned char)((from_block >> 8) & 0xFF);
    rdCmd[5] = (unsigned char)(from_block & 0xFF);
    rdCmd[7] = (unsigned char)((blocks >> 8) & 0xff);
    rdCmd[8] = (unsigned char)(blocks & 0xff);

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = sizeof(rdCmd);
    io_hdr.cmdp = rdCmd;
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = bs * blocks;
    io_hdr.dxferp = buff;
    io_hdr.mx_sb_len = SENSE_BUFF_LEN;
    io_hdr.sbp = senseBuff;
    io_hdr.timeout = DEF_TIMEOUT;
    io_hdr.pack_id = 0;
    if (diop && *diop)
        io_hdr.flags |= SG_FLAG_DIRECT_IO;

// fprintf(stderr," writing read command to device\n");
    while (((res = write(sg_fd, &io_hdr, sizeof(io_hdr))) < 0) &&
           (EINTR == errno))
        ;
    if (res < 0) {
        if (ENOMEM == errno)
            return 1;
        perror("reading (wr) on sg device, error");
        return -1;
    }

// fprintf(stderr," reading status from device\n");
    while (((res = read(sg_fd, &io_hdr, sizeof(io_hdr))) < 0) &&
           (EINTR == errno))
        ;
    if (res < 0) {
        perror("reading (rd) on sg device, error");
        return -1;
    }
    switch (sg_err_category3(&io_hdr)) {
    case SG_ERR_CAT_CLEAN:
        break;
    case SG_ERR_CAT_RECOVERED:
        fprintf(stderr, "Recovered error while reading block=%d, num=%d\n",
               from_block, blocks);
        break;
    case SG_ERR_CAT_MEDIA_CHANGED:
        return 2;
    default:
        fprintf(stderr,"read error");
        return -1;
    }
// fprintf(stderr,"Done with read \n");
    if (diop && *diop && 
        ((io_hdr.info & SG_INFO_DIRECT_IO_MASK) != SG_INFO_DIRECT_IO))
        *diop = 0;      /* flag that dio not done (completely) */
    sum_of_resids += io_hdr.resid;
#if SG_DEBUG
    fprintf(stderr, "duration=%u ms\n", io_hdr.duration);
#endif
    return(bs*blocks);
}

/* -1 -> unrecoverable error, 0 -> successful, 1 -> recoverable (ENOMEM),
   2 -> try again */
int xdd_sg_write(int sg_fd, unsigned char * buff, int blocks, int to_block, int bs, int * diop)
{
    unsigned char wrCmd[10] = {0x2a, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char senseBuff[SENSE_BUFF_LEN];
    sg_io_hdr_t io_hdr;
    int res;

    wrCmd[2] = (unsigned char)((to_block >> 24) & 0xFF);
    wrCmd[3] = (unsigned char)((to_block >> 16) & 0xFF);
    wrCmd[4] = (unsigned char)((to_block >> 8) & 0xFF);
    wrCmd[5] = (unsigned char)(to_block & 0xFF);
    wrCmd[7] = (unsigned char)((blocks >> 8) & 0xff);
    wrCmd[8] = (unsigned char)(blocks & 0xff);

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = sizeof(wrCmd);
    io_hdr.cmdp = wrCmd;
    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    io_hdr.dxfer_len = bs * blocks;
    io_hdr.dxferp = buff;
    io_hdr.mx_sb_len = SENSE_BUFF_LEN;
    io_hdr.sbp = senseBuff;
    io_hdr.timeout = DEF_TIMEOUT;
    io_hdr.pack_id = to_block;
    if (diop && *diop)
        io_hdr.flags |= SG_FLAG_DIRECT_IO;

    while (((res = write(sg_fd, &io_hdr, sizeof(io_hdr))) < 0) &&
           (EINTR == errno))
        ;
    if (res < 0) {
        if (ENOMEM == errno)
            return 1;
        perror("writing (wr) on sg device, error");
        return -1;
    }

    while (((res = read(sg_fd, &io_hdr, sizeof(io_hdr))) < 0) &&
           (EINTR == errno))
        ;
    if (res < 0) {
        perror("writing (rd) on sg device, error");
        return -1;
    }
    switch (sg_err_category3(&io_hdr)) {
    case SG_ERR_CAT_CLEAN:
        break;
    case SG_ERR_CAT_RECOVERED:
        fprintf(stderr, "Recovered error while writing block=%d, num=%d\n",
               to_block, blocks);
        break;
    case SG_ERR_CAT_MEDIA_CHANGED:
        return 2;
    default:
        fprintf(stderr,"write error");
        return -1;
    }
    if (diop && *diop && 
        ((io_hdr.info & SG_INFO_DIRECT_IO_MASK) != SG_INFO_DIRECT_IO))
        *diop = 0;      /* flag that dio not done (completely) */
    return(bs*blocks);
}

#ifdef ndef

int sg_main(int argc, char * argv[])
{
    int skip = 0;
    int seek = 0;
    int bs = 0;
    int ibs = 0;
    int obs = 0;
    int bpt = DEF_BLOCKS_PER_TRANSFER;
    char str[512];
    char * key;
    char * buf;
    char inf[512];
    int in_type = FT_OTHER;
    char outf[512];
    int out_type = FT_OTHER;
    int dio = 0;
    int dio_incomplete = 0;
    int res, k, t, buf_sz, dio_tmp;
    int infd, outfd, blocks;
    unsigned char * wrkBuff;
    unsigned char * wrkPos;
    int in_num_sect = 0;
    int out_num_sect = 0;
    int in_sect_sz, out_sect_sz;
    char ebuff[256];
    int blocks_per;

    inf[0] = '\0';
    outf[0] = '\0';
    if (argc < 2) {
        usage();
        return 1;
    }

    for(k = 1; k < argc; k++) {
        if (argv[k])
            strcpy(str, argv[k]);
        else
            continue;
        for(key = str, buf = key; *buf && *buf != '=';)
            buf++;
        if (*buf)
            *buf++ = '\0';
        if (strcmp(key,"if") == 0)
            strcpy(inf, buf);
        else if (strcmp(key,"of") == 0)
            strcpy(outf, buf);
        else if (0 == strcmp(key,"ibs"))
            ibs = get_num(buf);
        else if (0 == strcmp(key,"obs"))
            obs = get_num(buf);
        else if (0 == strcmp(key,"bs"))
            bs = get_num(buf);
        else if (0 == strcmp(key,"bpt"))
            bpt = get_num(buf);
        else if (0 == strcmp(key,"skip"))
            skip = get_num(buf);
        else if (0 == strcmp(key,"seek"))
            seek = get_num(buf);
        else if (0 == strcmp(key,"count"))
            dd_count = get_num(buf);
        else if (0 == strcmp(key,"dio"))
            dio = get_num(buf);
        else {
            fprintf(stderr, "Unrecognized argument '%s'\n", key);
            usage();
            return 1;
        }
    }
    if (bs <= 0) {
        bs = DEF_BLOCK_SIZE;
        fprintf(stderr, "Assume default 'bs' (block size) of %d bytes\n", bs);
    }
    if ((ibs && (ibs != bs)) || (obs && (obs != bs))) {
        fprintf(stderr, "If 'ibs' or 'obs' given must be same as 'bs'\n");
        usage();
        return 1;
    }
    if ((skip < 0) || (seek < 0)) {
        fprintf(stderr, "skip and seek cannot be negative\n");
        return 1;
    }
#ifdef SG_DEBUG
    fprintf(stderr, "sg_dd: if=%s skip=%d of=%s seek=%d count=%d\n",
           inf, skip, outf, seek, dd_count);
#endif
    install_handler (SIGINT, interrupt_handler);
    install_handler (SIGQUIT, interrupt_handler);
    install_handler (SIGPIPE, interrupt_handler);
    install_handler (SIGUSR1, siginfo_handler);

    infd = STDIN_FILENO;
    outfd = STDOUT_FILENO;
    if (inf[0] && ('-' != inf[0])) {
	in_type = dd_filetype(inf);

	if (FT_SG == in_type) {
	    if ((infd = open(inf, O_RDWR)) < 0) {
                sprintf(ebuff, "sg_dd: could not open %s for sg reading", inf);
                perror(ebuff);
                return 1;
            }
	    t = bs * bpt;
            res = ioctl(infd, SG_SET_RESERVED_SIZE, &t);
            if (res < 0)
                perror("sg_dd: SG_SET_RESERVED_SIZE error");
            res = ioctl(infd, SG_GET_VERSION_NUM, &t);
            if ((res < 0) || (t < 30000)) {
                fprintf(stderr, "sg_dd: sg driver prior to 3.x.y\n");
                return 1;
            }
        }
        if (FT_SG != in_type) {
            if ((infd = open(inf, O_RDONLY)) < 0) {
                sprintf(ebuff, "sg_dd: could not open %s for reading", inf);
                perror(ebuff);
                return 1;
            }
            else if (skip > 0) {
                llse_loff_t offset = skip;

                offset *= bs;       // could exceed 32 bits here!
                if (llse_llseek(infd, offset, SEEK_SET) < 0) {
                    sprintf(ebuff,
                "sg_dd: couldn't skip to required position on %s", inf);
                    perror(ebuff);
                    return 1;
                }
            }
        }
    }

    if (outf[0] && ('-' != outf[0])) {
	out_type = dd_filetype(outf);

	if (FT_SG == out_type) {
	    if ((outfd = open(outf, O_RDWR)) < 0) {
                sprintf(ebuff, "sg_dd: could not open %s for sg writing", outf);
                perror(ebuff);
                return 1;
            }
            t = bs * bpt;
            res = ioctl(outfd, SG_SET_RESERVED_SIZE, &t);
            if (res < 0)
                perror("sg_dd: SG_SET_RESERVED_SIZE error");
            res = ioctl(outfd, SG_GET_VERSION_NUM, &t);
            if ((res < 0) || (t < 30000)) {
                fprintf(stderr, "sg_dd: sg driver prior to 3.x.y\n");
                return 1;
            }
        }
	else {
	    if (FT_OTHER == out_type) {
		if ((outfd = open(outf, O_WRONLY | O_CREAT, 0666)) < 0) {
		    sprintf(ebuff,
			    "sg_dd: could not open %s for writing", outf);
		    perror(ebuff);
		    return 1;
		}
	    }
	    else {
		if ((outfd = open(outf, O_WRONLY)) < 0) {
		    sprintf(ebuff,
			    "sg_dd: could not open %s for raw writing", outf);
		    perror(ebuff);
		    return 1;
		}
	    }
            if (seek > 0) {
                llse_loff_t offset = seek;

                offset *= bs;       // could exceed 32 bits here!
                if (llse_llseek(outfd, offset, SEEK_SET) < 0) {
                    sprintf(ebuff,
                "sg_dd: couldn't seek to required position on %s", outf);
                    perror(ebuff);
                    return 1;
                }
            }
        }
    }
    if ((STDIN_FILENO == infd) && (STDOUT_FILENO == outfd)) {
        fprintf(stderr, 
		"Can't have both 'if' as stdin _and_ 'of' as stdout\n");
        return 1;
    }
#if 1
    if ((FT_OTHER == in_type) && (FT_OTHER == out_type)) {
        fprintf(stderr, "Both 'if' and 'of' can't be ordinary files\n");
        return 1;
    }
#endif
    if (0 == dd_count)
        return 0;
    else if (dd_count < 0) {
        if (FT_SG == in_type) {
            res = read_capacity(infd, &in_num_sect, &in_sect_sz);
            if (2 == res) {
                fprintf(stderr, 
			"Unit attention, media changed(in), continuing\n");
                res = read_capacity(infd, &in_num_sect, &in_sect_sz);
            }
            if (0 != res) {
                fprintf(stderr, "Unable to read capacity on %s\n", inf);
                in_num_sect = -1;
            }
            else {
#if 0
                if (0 == in_sect_sz)
                    in_sect_sz = bs;
                else if (in_sect_sz > bs)
                    in_num_sect *=  (in_sect_sz / bs);
                else if (in_sect_sz < bs)
                    in_num_sect /=  (bs / in_sect_sz);
#endif
                if (in_num_sect > skip)
                    in_num_sect -= skip;
            }
        }
        if (FT_SG == out_type) {
            res = read_capacity(outfd, &out_num_sect, &out_sect_sz);
            if (2 == res) {
                fprintf(stderr, 
			"Unit attention, media changed(out), continuing\n");
                res = read_capacity(outfd, &out_num_sect, &out_sect_sz);
            }
            if (0 != res) {
                fprintf(stderr, "Unable to read capacity on %s\n", outf);
                out_num_sect = -1;
            }
            else {
                if (out_num_sect > seek)
                    out_num_sect -= seek;
            }
        }
#ifdef SG_DEBUG
    fprintf(stderr, 
	    "Start of loop, count=%d, in_num_sect=%d, out_num_sect=%d\n", 
            dd_count, in_num_sect, out_num_sect);
#endif
        if (in_num_sect > 0) {
            if (out_num_sect > 0)
                dd_count = (in_num_sect > out_num_sect) ? out_num_sect :
                                                       in_num_sect;
            else
                dd_count = in_num_sect;
        }
        else
            dd_count = out_num_sect;
    }
    if (dd_count <= 0) {
        fprintf(stderr, "Couldn't calculate count, please give one\n");
        return 1;
    }

    if (dio || (FT_RAW == in_type) || (FT_RAW == out_type)) {
	size_t psz = getpagesize();
	wrkBuff = malloc(bs * bpt + psz);
	if (0 == wrkBuff) {
            fprintf(stderr, "Not enough user memory for raw\n");
            return 1;
        }
	wrkPos = (unsigned char *)(((unsigned long)wrkBuff + psz - 1) &
		                   (~(psz - 1)));
    }
    else {
	wrkBuff = malloc(bs * bpt);
	if (0 == wrkBuff) {
            fprintf(stderr, "Not enough user memory\n");
            return 1;
        }
        wrkPos = wrkBuff;
    }

    blocks_per = bpt;
#ifdef SG_DEBUG
    fprintf(stderr, "Start of loop, count=%d, blocks_per=%d\n", 
	    dd_count, blocks_per);
#endif
    while (dd_count > 0) {
        blocks = (dd_count > blocks_per) ? blocks_per : dd_count;
        if (FT_SG == in_type) {
            dio_tmp = dio;
            res = sg_read(infd, wrkPos, blocks, skip, bs, &dio_tmp);
            if (1 == res) {     /* ENOMEM, find what's available+try that */
                if (ioctl(infd, SG_GET_RESERVED_SIZE, &buf_sz) < 0) {
                    perror("RESERVED_SIZE ioctls failed");
                    break;
                }
                blocks_per = (buf_sz + bs - 1) / bs;
                blocks = blocks_per;
                fprintf(stderr, 
			"Reducing read to %d blocks per loop\n", blocks_per);
                res = sg_read(infd, wrkPos, blocks, skip, bs, &dio_tmp);
            }
            else if (2 == res) {
                fprintf(stderr, 
			"Unit attention, media changed, continuing (r)\n");
                res = sg_read(infd, wrkPos, blocks, skip, bs, &dio_tmp);
            }
            if (0 != res) {
                fprintf(stderr, "sg_read failed, skip=%d\n", skip);
                break;
            }
            else {
                in_full += blocks;
                if (dio && (0 == dio_tmp))
                    dio_incomplete++;
            }
        }
        else {
	    while (((res = read(infd, wrkPos, blocks * bs)) < 0) &&
		   (EINTR == errno))
		;
            if (res < 0) {
                sprintf(ebuff, "sg_dd: reading, skip=%d ", skip);
                perror(ebuff);
                break;
            }
            else if (res < blocks * bs) {
                dd_count = 0;
                blocks = res / bs;
                if ((res % bs) > 0) {
                    blocks++;
                    in_partial++;
                }
            }
            in_full += blocks;
        }

        if (FT_SG == out_type) {
            dio_tmp = dio;
            res = sg_write(outfd, wrkPos, blocks, seek, bs, &dio_tmp);
            if (1 == res) {     /* ENOMEM, find what's available+try that */
                if (ioctl(outfd, SG_GET_RESERVED_SIZE, &buf_sz) < 0) {
                    perror("RESERVED_SIZE ioctls failed");
                    break;
                }
                blocks_per = (buf_sz + bs - 1) / bs;
                blocks = blocks_per;
                fprintf(stderr, 
			"Reducing write to %d blocks per loop\n", blocks);
                res = sg_write(outfd, wrkPos, blocks, seek, bs, &dio_tmp);
            }
            else if (2 == res) {
                fprintf(stderr, 
			"Unit attention, media changed, continuing (w)\n");
                res = sg_write(outfd, wrkPos, blocks, seek, bs, &dio_tmp);
            }
            else if (0 != res) {
                fprintf(stderr, "sg_write failed, seek=%d\n", seek);
                break;
            }
            else {
                out_full += blocks;
                if (dio && (0 == dio_tmp))
                    dio_incomplete++;
            }
        }
        else {
	    while (((res = write(outfd, wrkPos, blocks * bs)) < 0)
		   && (EINTR == errno))
		;
            if (res < 0) {
                sprintf(ebuff, "sg_dd: writing, seek=%d ", seek);
                perror(ebuff);
                break;
            }
            else if (res < blocks * bs) {
                fprintf(stderr, "output file probably full, seek=%d ", seek);
                blocks = res / bs;
                out_full += blocks;
                if ((res % bs) > 0)
                    out_partial++;
                break;
            }
            else
                out_full += blocks;
        }
        if (dd_count > 0)
            dd_count -= blocks;
        skip += blocks;
        seek += blocks;
    }

    free(wrkBuff);
    if (STDIN_FILENO != infd)
        close(infd);
    if (STDOUT_FILENO != outfd)
        close(outfd);
    res = 0;
    if (0 != dd_count) {
        fprintf(stderr, "Some error occurred,");
	res = 2;
    }
    print_stats();
    if (dio_incomplete) {
    	int fd;
	char c;

        fprintf(stderr, ">> Direct IO requested but incomplete %d times\n", 
                dio_incomplete);
	if ((fd = open(proc_allow_dio, O_RDONLY)) >= 0) {
	    if (1 == read(fd, &c, 1)) {
	    	if ('0' == c)
		    fprintf(stderr, ">>> %s set to '0' but should be set "
		    	    "to '1' for direct IO\n", proc_allow_dio);
	    }
	    close(fd);
	}
    }
    if (sum_of_resids)
        fprintf(stderr, ">> Non-zero sum of residual counts=%d\n", 
		sum_of_resids);
    return res;
}
#endif



/* This file is a huge cut, paste and hack from linux/drivers/scsi/constant.c
*  which I guess was written by:
*         Copyright (C) 1993, 1994, 1995 Eric Youngdale

* The rest of this is:
*  Copyright (C) 1999 - 2001 D. Gilbert
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  ASCII values for a number of symbolic constants, printing functions, etc.
*
*  Some of the tables have been updated for SCSI 2.
*
*  Version 0.84 (20010115)
*      Change output from stdout to stderr
*/

#define OUTP stderr

static const unsigned char scsi_command_size[8] = { 6, 10, 10, 12,
                                                   12, 12, 10, 10 };

#define COMMAND_SIZE(opcode) scsi_command_size[((opcode) >> 5) & 7]

static const char unknown[] = "UNKNOWN";

static const char * group_0_commands[] = {
/* 00-03 */ "Test Unit Ready", "Rezero Unit", unknown, "Request Sense",
/* 04-07 */ "Format Unit", "Read Block Limits", unknown, "Reasssign Blocks",
/* 08-0d */ "Read (6)", unknown, "Write (6)", "Seek (6)", unknown, unknown,
/* 0e-12 */ unknown, "Read Reverse", "Write Filemarks", "Space", "Inquiry",
/* 13-16 */ "Verify", "Recover Buffered Data", "Mode Select", "Reserve",
/* 17-1b */ "Release", "Copy", "Erase", "Mode Sense", "Start/Stop Unit",
/* 1c-1d */ "Receive Diagnostic", "Send Diagnostic",
/* 1e-1f */ "Prevent/Allow Medium Removal", unknown,
};


static const char *group_1_commands[] = {
/* 20-22 */  unknown, unknown, unknown,
/* 23-28 */ unknown, "Define window parameters", "Read Capacity",
            unknown, unknown, "Read (10)",
/* 29-2d */ "Read Generation", "Write (10)", "Seek (10)", "Erase",
            "Read updated block",
/* 2e-31 */ "Write Verify","Verify", "Search High", "Search Equal",
/* 32-34 */ "Search Low", "Set Limits", "Prefetch or Read Position",
/* 35-37 */ "Synchronize Cache","Lock/Unlock Cache", "Read Defect Data",
/* 38-3c */ "Medium Scan", "Compare", "Copy Verify", "Write Buffer",
            "Read Buffer",
/* 3d-3f */ "Update Block", "Read Long",  "Write Long",
};

static const char *group_2_commands[] = {
/* 40-41 */ "Change Definition", "Write Same",
/* 42-48 */ "Read sub-channel", "Read TOC", "Read header",
            "Play audio (10)", unknown, "Play audio msf",
            "Play audio track/index",
/* 49-4f */ "Play track relative (10)", unknown, "Pause/resume",
            "Log Select", "Log Sense", unknown, unknown,
/* 50-55 */ unknown, unknown, unknown, unknown, unknown, "Mode Select (10)",
/* 56-5b */ unknown, unknown, unknown, unknown, "Mode Sense (10)", unknown,
/* 5c-5f */ unknown, unknown, unknown,
};


/* The following are 12 byte commands in group 5 */
static const char *group_5_commands[] = {
/* a0-a5 */ unknown, unknown, unknown, unknown, unknown,
            "Move medium/play audio(12)",
/* a6-a9 */ "Exchange medium", unknown, "Read(12)", "Play track relative(12)",
/* aa-ae */ "Write(12)", unknown, "Erase(12)", unknown,
            "Write and verify(12)",
/* af-b1 */ "Verify(12)", "Search data high(12)", "Search data equal(12)",
/* b2-b4 */ "Search data low(12)", "Set limits(12)", unknown,
/* b5-b6 */ "Request volume element address", "Send volume tag",
/* b7-b9 */ "Read defect data(12)", "Read element status", unknown,
/* ba-bf */ unknown, unknown, unknown, unknown, unknown, unknown,
};




#define group(opcode) (((opcode) >> 5) & 7)

#define RESERVED_GROUP  0
#define VENDOR_GROUP    1

static const char **commands[] = {
    group_0_commands, group_1_commands, group_2_commands,
    (const char **) RESERVED_GROUP, (const char **) RESERVED_GROUP,
    group_5_commands, (const char **) VENDOR_GROUP,
    (const char **) VENDOR_GROUP
};

static const char reserved[] = "RESERVED";
static const char vendor[] = "VENDOR SPECIFIC";

static void print_opcode(int opcode) {
    const char **table = commands[ group(opcode) ];
    switch ((unsigned long) table) {
    case RESERVED_GROUP:
        fprintf(OUTP, "%s(0x%02x) ", reserved, opcode);
        break;
    case VENDOR_GROUP:
        fprintf(OUTP, "%s(0x%02x) ", vendor, opcode);
        break;
    default:
        if (table[opcode & 0x1f] != unknown)
            fprintf(OUTP, "%s ",table[opcode & 0x1f]);
        else
            fprintf(OUTP, "%s(0x%02x) ", unknown, opcode);
        break;
    }
}

void sg_print_command (const unsigned char * command) {
    int i,s;
    print_opcode(command[0]);
    for ( i = 1, s = COMMAND_SIZE(command[0]); i < s; ++i)
        fprintf(OUTP, "%02x ", command[i]);
    fprintf(OUTP, "\n");
}

static const char * statuses[] = {
/* 0-4 */ "Good", "Check Condition", "Condition Met", unknown, "Busy",
/* 5-9 */ unknown, unknown, unknown, "Intermediate", unknown,
/* a-c */ "Intermediate-Condition Met", unknown, "Reservation Conflict",
/* d-10 */ unknown, unknown, unknown, unknown,
/* 11-14 */ "Command Terminated", unknown, unknown, "Queue Full",
/* 15-1a */ unknown, unknown, unknown,  unknown, unknown, unknown,
/* 1b-1f */ unknown, unknown, unknown,  unknown, unknown,
};

void sg_print_status (int masked_status) {
    /* status = (status >> 1) & 0xf; */ /* already done */
    fprintf(OUTP, "%s ",statuses[masked_status]);
}

#define D 0x001  /* DIRECT ACCESS DEVICE (disk) */
#define T 0x002  /* SEQUENTIAL ACCESS DEVICE (tape) */
#define L 0x004  /* PRINTER DEVICE */
#define P 0x008  /* PROCESSOR DEVICE */
#define W 0x010  /* WRITE ONCE READ MULTIPLE DEVICE */
#define R 0x020  /* READ ONLY (CD-ROM) DEVICE */
#define S 0x040  /* SCANNER DEVICE */
#define O 0x080  /* OPTICAL MEMORY DEVICE */
#define M 0x100  /* MEDIA CHANGER DEVICE */
#define C 0x200  /* COMMUNICATION DEVICE */

struct error_info{
    unsigned char code1, code2;
    unsigned short int devices;
    const char * text;
};

struct error_info2{
    unsigned char code1, code2_min, code2_max;
    unsigned short int devices;
    const char * text;
};

static struct error_info2 additional2[] =
{
  {0x40,0x00,0x7f,D,"Ram failure (%x)"},
  {0x40,0x80,0xff,D|T|L|P|W|R|S|O|M|C,"Diagnostic failure on component (%x)"},
  {0x41,0x00,0xff,D,"Data path failure (%x)"},
  {0x42,0x00,0xff,D,"Power-on or self-test failure (%x)"},
  {0, 0, 0, 0, NULL}
};

static struct error_info additional[] =
{
  {0x00,0x01,T,"Filemark detected"},
  {0x00,0x02,T|S,"End-of-partition/medium detected"},
  {0x00,0x03,T,"Setmark detected"},
  {0x00,0x04,T|S,"Beginning-of-partition/medium detected"},
  {0x00,0x05,T|S,"End-of-data detected"},
  {0x00,0x06,D|T|L|P|W|R|S|O|M|C,"I/O process terminated"},
  {0x00,0x11,R,"Audio play operation in progress"},
  {0x00,0x12,R,"Audio play operation paused"},
  {0x00,0x13,R,"Audio play operation successfully completed"},
  {0x00,0x14,R,"Audio play operation stopped due to error"},
  {0x00,0x15,R,"No current audio status to return"},
  {0x01,0x00,D|W|O,"No index/sector signal"},
  {0x02,0x00,D|W|R|O|M,"No seek complete"},
  {0x03,0x00,D|T|L|W|S|O,"Peripheral device write fault"},
  {0x03,0x01,T,"No write current"},
  {0x03,0x02,T,"Excessive write errors"},
  {0x04,0x00,D|T|L|P|W|R|S|O|M|C,
     "Logical unit not ready, cause not reportable"},
  {0x04,0x01,D|T|L|P|W|R|S|O|M|C,
     "Logical unit is in process of becoming ready"},
  {0x04,0x02,D|T|L|P|W|R|S|O|M|C,
     "Logical unit not ready, initializing command required"},
  {0x04,0x03,D|T|L|P|W|R|S|O|M|C,
     "Logical unit not ready, manual intervention required"},
  {0x04,0x04,D|T|L|O,"Logical unit not ready, format in progress"},
  {0x05,0x00,D|T|L|W|R|S|O|M|C,"Logical unit does not respond to selection"},
  {0x06,0x00,D|W|R|O|M,"No reference position found"},
  {0x07,0x00,D|T|L|W|R|S|O|M,"Multiple peripheral devices selected"},
  {0x08,0x00,D|T|L|W|R|S|O|M|C,"Logical unit communication failure"},
  {0x08,0x01,D|T|L|W|R|S|O|M|C,"Logical unit communication time-out"},
  {0x08,0x02,D|T|L|W|R|S|O|M|C,"Logical unit communication parity error"},
  {0x09,0x00,D|T|W|R|O,"Track following error"},
  {0x09,0x01,W|R|O,"Tracking servo failure"},
  {0x09,0x02,W|R|O,"Focus servo failure"},
  {0x09,0x03,W|R|O,"Spindle servo failure"},
  {0x0A,0x00,D|T|L|P|W|R|S|O|M|C,"Error log overflow"},
  {0x0C,0x00,T|S,"Write error"},
  {0x0C,0x01,D|W|O,"Write error recovered with auto reallocation"},
  {0x0C,0x02,D|W|O,"Write error - auto reallocation failed"},
  {0x10,0x00,D|W|O,"Id crc or ecc error"},
  {0x11,0x00,D|T|W|R|S|O,"Unrecovered read error"},
  {0x11,0x01,D|T|W|S|O,"Read retries exhausted"},
  {0x11,0x02,D|T|W|S|O,"Error too long to correct"},
  {0x11,0x03,D|T|W|S|O,"Multiple read errors"},
  {0x11,0x04,D|W|O,"Unrecovered read error - auto reallocate failed"},
  {0x11,0x05,W|R|O,"L-ec uncorrectable error"},
  {0x11,0x06,W|R|O,"Circ unrecovered error"},
  {0x11,0x07,W|O,"Data resynchronization error"},
  {0x11,0x08,T,"Incomplete block read"},
  {0x11,0x09,T,"No gap found"},
  {0x11,0x0A,D|T|O,"Miscorrected error"},
  {0x11,0x0B,D|W|O,"Unrecovered read error - recommend reassignment"},
  {0x11,0x0C,D|W|O,"Unrecovered read error - recommend rewrite the data"},
  {0x12,0x00,D|W|O,"Address mark not found for id field"},
  {0x13,0x00,D|W|O,"Address mark not found for data field"},
  {0x14,0x00,D|T|L|W|R|S|O,"Recorded entity not found"},
  {0x14,0x01,D|T|W|R|O,"Record not found"},
  {0x14,0x02,T,"Filemark or setmark not found"},
  {0x14,0x03,T,"End-of-data not found"},
  {0x14,0x04,T,"Block sequence error"},
  {0x15,0x00,D|T|L|W|R|S|O|M,"Random positioning error"},
  {0x15,0x01,D|T|L|W|R|S|O|M,"Mechanical positioning error"},
  {0x15,0x02,D|T|W|R|O,"Positioning error detected by read of medium"},
  {0x16,0x00,D|W|O,"Data synchronization mark error"},
  {0x17,0x00,D|T|W|R|S|O,"Recovered data with no error correction applied"},
  {0x17,0x01,D|T|W|R|S|O,"Recovered data with retries"},
  {0x17,0x02,D|T|W|R|O,"Recovered data with positive head offset"},
  {0x17,0x03,D|T|W|R|O,"Recovered data with negative head offset"},
  {0x17,0x04,W|R|O,"Recovered data with retries and/or circ applied"},
  {0x17,0x05,D|W|R|O,"Recovered data using previous sector id"},
  {0x17,0x06,D|W|O,"Recovered data without ecc - data auto-reallocated"},
  {0x17,0x07,D|W|O,"Recovered data without ecc - recommend reassignment"},
  {0x18,0x00,D|T|W|R|O,"Recovered data with error correction applied"},
  {0x18,0x01,D|W|R|O,"Recovered data with error correction and retries applied"},
  {0x18,0x02,D|W|R|O,"Recovered data - data auto-reallocated"},
  {0x18,0x03,R,"Recovered data with circ"},
  {0x18,0x04,R,"Recovered data with lec"},
  {0x18,0x05,D|W|R|O,"Recovered data - recommend reassignment"},
  {0x19,0x00,D|O,"Defect list error"},
  {0x19,0x01,D|O,"Defect list not available"},
  {0x19,0x02,D|O,"Defect list error in primary list"},
  {0x19,0x03,D|O,"Defect list error in grown list"},
  {0x1A,0x00,D|T|L|P|W|R|S|O|M|C,"Parameter list length error"},
  {0x1B,0x00,D|T|L|P|W|R|S|O|M|C,"Synchronous data transfer error"},
  {0x1C,0x00,D|O,"Defect list not found"},
  {0x1C,0x01,D|O,"Primary defect list not found"},
  {0x1C,0x02,D|O,"Grown defect list not found"},
  {0x1D,0x00,D|W|O,"Miscompare during verify operation"},
  {0x1E,0x00,D|W|O,"Recovered id with ecc correction"},
  {0x20,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid command operation code"},
  {0x21,0x00,D|T|W|R|O|M,"Logical block address out of range"},
  {0x21,0x01,M,"Invalid element address"},
  {0x22,0x00,D,"Illegal function (should use 20 00, 24 00, or 26 00)"},
  {0x24,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid field in cdb"},
  {0x25,0x00,D|T|L|P|W|R|S|O|M|C,"Logical unit not supported"},
  {0x26,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid field in parameter list"},
  {0x26,0x01,D|T|L|P|W|R|S|O|M|C,"Parameter not supported"},
  {0x26,0x02,D|T|L|P|W|R|S|O|M|C,"Parameter value invalid"},
  {0x26,0x03,D|T|L|P|W|R|S|O|M|C,"Threshold parameters not supported"},
  {0x27,0x00,D|T|W|O,"Write protected"},
  {0x28,0x00,D|T|L|P|W|R|S|O|M|C,"Not ready to ready transition (medium may have changed)"},
  {0x28,0x01,M,"Import or export element accessed"},
  {0x29,0x00,D|T|L|P|W|R|S|O|M|C,"Power on, reset, or bus device reset occurred"},
  {0x2A,0x00,D|T|L|W|R|S|O|M|C,"Parameters changed"},
  {0x2A,0x01,D|T|L|W|R|S|O|M|C,"Mode parameters changed"},
  {0x2A,0x02,D|T|L|W|R|S|O|M|C,"Log parameters changed"},
  {0x2B,0x00,D|T|L|P|W|R|S|O|C,"Copy cannot execute since host cannot disconnect"},
  {0x2C,0x00,D|T|L|P|W|R|S|O|M|C,"Command sequence error"},
  {0x2C,0x01,S,"Too many windows specified"},
  {0x2C,0x02,S,"Invalid combination of windows specified"},
  {0x2D,0x00,T,"Overwrite error on update in place"},
  {0x2F,0x00,D|T|L|P|W|R|S|O|M|C,"Commands cleared by another initiator"},
  {0x30,0x00,D|T|W|R|O|M,"Incompatible medium installed"},
  {0x30,0x01,D|T|W|R|O,"Cannot read medium - unknown format"},
  {0x30,0x02,D|T|W|R|O,"Cannot read medium - incompatible format"},
  {0x30,0x03,D|T,"Cleaning cartridge installed"},
  {0x31,0x00,D|T|W|O,"Medium format corrupted"},
  {0x31,0x01,D|L|O,"Format command failed"},
  {0x32,0x00,D|W|O,"No defect spare location available"},
  {0x32,0x01,D|W|O,"Defect list update failure"},
  {0x33,0x00,T,"Tape length error"},
  {0x36,0x00,L,"Ribbon, ink, or toner failure"},
  {0x37,0x00,D|T|L|W|R|S|O|M|C,"Rounded parameter"},
  {0x39,0x00,D|T|L|W|R|S|O|M|C,"Saving parameters not supported"},
  {0x3A,0x00,D|T|L|W|R|S|O|M,"Medium not present"},
  {0x3B,0x00,T|L,"Sequential positioning error"},
  {0x3B,0x01,T,"Tape position error at beginning-of-medium"},
  {0x3B,0x02,T,"Tape position error at end-of-medium"},
  {0x3B,0x03,L,"Tape or electronic vertical forms unit not ready"},
  {0x3B,0x04,L,"Slew failure"},
  {0x3B,0x05,L,"Paper jam"},
  {0x3B,0x06,L,"Failed to sense top-of-form"},
  {0x3B,0x07,L,"Failed to sense bottom-of-form"},
  {0x3B,0x08,T,"Reposition error"},
  {0x3B,0x09,S,"Read past end of medium"},
  {0x3B,0x0A,S,"Read past beginning of medium"},
  {0x3B,0x0B,S,"Position past end of medium"},
  {0x3B,0x0C,S,"Position past beginning of medium"},
  {0x3B,0x0D,M,"Medium destination element full"},
  {0x3B,0x0E,M,"Medium source element empty"},
  {0x3D,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid bits in identify message"},
  {0x3E,0x00,D|T|L|P|W|R|S|O|M|C,"Logical unit has not self-configured yet"},
  {0x3F,0x00,D|T|L|P|W|R|S|O|M|C,"Target operating conditions have changed"},
  {0x3F,0x01,D|T|L|P|W|R|S|O|M|C,"Microcode has been changed"},
  {0x3F,0x02,D|T|L|P|W|R|S|O|M|C,"Changed operating definition"},
  {0x3F,0x03,D|T|L|P|W|R|S|O|M|C,"Inquiry data has changed"},
  {0x43,0x00,D|T|L|P|W|R|S|O|M|C,"Message error"},
  {0x44,0x00,D|T|L|P|W|R|S|O|M|C,"Internal target failure"},
  {0x45,0x00,D|T|L|P|W|R|S|O|M|C,"Select or reselect failure"},
  {0x46,0x00,D|T|L|P|W|R|S|O|M|C,"Unsuccessful soft reset"},
  {0x47,0x00,D|T|L|P|W|R|S|O|M|C,"Scsi parity error"},
  {0x48,0x00,D|T|L|P|W|R|S|O|M|C,"Initiator detected error message received"},
  {0x49,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid message error"},
  {0x4A,0x00,D|T|L|P|W|R|S|O|M|C,"Command phase error"},
  {0x4B,0x00,D|T|L|P|W|R|S|O|M|C,"Data phase error"},
  {0x4C,0x00,D|T|L|P|W|R|S|O|M|C,"Logical unit failed self-configuration"},
  {0x4E,0x00,D|T|L|P|W|R|S|O|M|C,"Overlapped commands attempted"},
  {0x50,0x00,T,"Write append error"},
  {0x50,0x01,T,"Write append position error"},
  {0x50,0x02,T,"Position error related to timing"},
  {0x51,0x00,T|O,"Erase failure"},
  {0x52,0x00,T,"Cartridge fault"},
  {0x53,0x00,D|T|L|W|R|S|O|M,"Media load or eject failed"},
  {0x53,0x01,T,"Unload tape failure"},
  {0x53,0x02,D|T|W|R|O|M,"Medium removal prevented"},
  {0x54,0x00,P,"Scsi to host system interface failure"},
  {0x55,0x00,P,"System resource failure"},
  {0x57,0x00,R,"Unable to recover table-of-contents"},
  {0x58,0x00,O,"Generation does not exist"},
  {0x59,0x00,O,"Updated block read"},
  {0x5A,0x00,D|T|L|P|W|R|S|O|M,"Operator request or state change input (unspecified)"},
  {0x5A,0x01,D|T|W|R|O|M,"Operator medium removal request"},
  {0x5A,0x02,D|T|W|O,"Operator selected write protect"},
  {0x5A,0x03,D|T|W|O,"Operator selected write permit"},
  {0x5B,0x00,D|T|L|P|W|R|S|O|M,"Log exception"},
  {0x5B,0x01,D|T|L|P|W|R|S|O|M,"Threshold condition met"},
  {0x5B,0x02,D|T|L|P|W|R|S|O|M,"Log counter at maximum"},
  {0x5B,0x03,D|T|L|P|W|R|S|O|M,"Log list codes exhausted"},
  {0x5C,0x00,D|O,"Rpl status change"},
  {0x5C,0x01,D|O,"Spindles synchronized"},
  {0x5C,0x02,D|O,"Spindles not synchronized"},
  {0x60,0x00,S,"Lamp failure"},
  {0x61,0x00,S,"Video acquisition error"},
  {0x61,0x01,S,"Unable to acquire video"},
  {0x61,0x02,S,"Out of focus"},
  {0x62,0x00,S,"Scan head positioning error"},
  {0x63,0x00,R,"End of user area encountered on this track"},
  {0x64,0x00,R,"Illegal mode for this track"},
  {0, 0, 0, NULL}
};

static const char *snstext[] = {
    "None",                     /* There is no sense information */
    "Recovered Error",          /* The last command completed successfully
                                   but used error correction */
    "Not Ready",                /* The addressed target is not ready */
    "Medium Error",             /* Data error detected on the medium */
    "Hardware Error",           /* Controller or device failure */
    "Illegal Request",
    "Unit Attention",           /* Removable medium was changed, or
                                   the target has been reset */
    "Data Protect",             /* Access to the data is blocked */
    "Blank Check",              /* Reached unexpected written or unwritten
                                   region of the medium */
    "Key=9",                    /* Vendor specific */
    "Copy Aborted",             /* COPY or COMPARE was aborted */
    "Aborted Command",          /* The target aborted the command */
    "Equal",                    /* A SEARCH DATA command found data equal */
    "Volume Overflow",          /* Medium full with still data to be written */
    "Miscompare",               /* Source data and data on the medium
                                   do not agree */
    "Key=15"                    /* Reserved */
};

/* Print sense information */
void sg_print_sense(const char * leadin, const unsigned char * sense_buffer,
                    int sb_len)
{
    int i, s;
    int sense_class, valid, code;
    const char * error = NULL;

    sense_class = (sense_buffer[0] >> 4) & 0x07;
    code = sense_buffer[0] & 0xf;
    valid = sense_buffer[0] & 0x80;

    if (sense_class == 7) {     /* extended sense data */
        s = sense_buffer[7] + 8;
        if(s > sb_len)
           s = sb_len;

        if (!valid)
            fprintf(OUTP, "[valid=0] ");
        fprintf(OUTP, "Info fld=0x%x, ", (int)((sense_buffer[3] << 24) |
                (sense_buffer[4] << 16) | (sense_buffer[5] << 8) |
                sense_buffer[6]));

        if (sense_buffer[2] & 0x80)
           fprintf(OUTP, "FMK ");     /* current command has read a filemark */
        if (sense_buffer[2] & 0x40)
           fprintf(OUTP, "EOM ");     /* end-of-medium condition exists */
        if (sense_buffer[2] & 0x20)
           fprintf(OUTP, "ILI ");     /* incorrect block length requested */

        switch (code) {
        case 0x0:
            error = "Current";  /* error concerns current command */
            break;
        case 0x1:
            error = "Deferred"; /* error concerns some earlier command */
                /* e.g., an earlier write to disk cache succeeded, but
                   now the disk discovers that it cannot write the data */
            break;
        default:
            error = "Invalid";
        }

        fprintf(OUTP, "%s ", error);

        if (leadin)
            fprintf(OUTP, "%s: ", leadin);
        fprintf(OUTP, "sense key: %s\n", snstext[sense_buffer[2] & 0x0f]);

        /* Check to see if additional sense information is available */
        if(sense_buffer[7] + 7 < 13 ||
           (sense_buffer[12] == 0  && sense_buffer[13] ==  0)) goto done;

        for(i=0; additional[i].text; i++)
            if(additional[i].code1 == sense_buffer[12] &&
               additional[i].code2 == sense_buffer[13])
                fprintf(OUTP, "Additional sense indicates: %s\n",
                        additional[i].text);

        for(i=0; additional2[i].text; i++)
            if(additional2[i].code1 == sense_buffer[12] &&
               additional2[i].code2_min >= sense_buffer[13]  &&
               additional2[i].code2_max <= sense_buffer[13]) {
                fprintf(OUTP, "Additional sense indicates: ");
                fprintf(OUTP, additional2[i].text, sense_buffer[13]);
                fprintf(OUTP, "\n");
            };
    } else {    /* non-extended sense data */

         /*
          * Standard says:
          *    sense_buffer[0] & 0200 : address valid
          *    sense_buffer[0] & 0177 : vendor-specific error code
          *    sense_buffer[1] & 0340 : vendor-specific
          *    sense_buffer[1..3] : 21-bit logical block address
          */

        if (leadin)
            fprintf(OUTP, "%s: ", leadin);
        if (sense_buffer[0] < 15)
            fprintf(OUTP, 
	    	    "old sense: key %s\n", snstext[sense_buffer[0] & 0x0f]);
        else
            fprintf(OUTP, "sns = %2x %2x\n", sense_buffer[0], sense_buffer[2]);

        fprintf(OUTP, "Non-extended sense class %d code 0x%0x ", 
		sense_class, code);
        s = 4;
    }

 done:
    fprintf(OUTP, "Raw sense data (in hex):\n  ");
    for (i = 0; i < s; ++i) {
        if ((i > 0) && (0 == (i % 24)))
            fprintf(OUTP, "\n  ");
        fprintf(OUTP, "%02x ", sense_buffer[i]);
    }
    fprintf(OUTP, "\n");
    return;
}

static const char * hostbyte_table[]={
"DID_OK", "DID_NO_CONNECT", "DID_BUS_BUSY", "DID_TIME_OUT", "DID_BAD_TARGET",
"DID_ABORT", "DID_PARITY", "DID_ERROR", "DID_RESET", "DID_BAD_INTR",
"DID_PASSTHROUGH", "DID_SOFT_ERROR", NULL};

void sg_print_host_status(int host_status)
{   static int maxcode=0;
    int i;

    if(! maxcode) {
        for(i = 0; hostbyte_table[i]; i++) ;
        maxcode = i-1;
    }
    fprintf(OUTP, "Host_status=0x%02x", host_status);
    if(host_status > maxcode) {
        fprintf(OUTP, "is invalid ");
        return;
    }
    fprintf(OUTP, "(%s) ",hostbyte_table[host_status]);
}

static const char * driverbyte_table[]={
"DRIVER_OK", "DRIVER_BUSY", "DRIVER_SOFT",  "DRIVER_MEDIA", "DRIVER_ERROR",
"DRIVER_INVALID", "DRIVER_TIMEOUT", "DRIVER_HARD", "DRIVER_SENSE", NULL};

static const char * driversuggest_table[]={"SUGGEST_OK",
"SUGGEST_RETRY", "SUGGEST_ABORT", "SUGGEST_REMAP", "SUGGEST_DIE",
unknown,unknown,unknown, "SUGGEST_SENSE",NULL};


void sg_print_driver_status(int driver_status)
{
    static int driver_max =0 , suggest_max=0;
    int i;
    int dr = driver_status & SG_ERR_DRIVER_MASK;
    int su = (driver_status & SG_ERR_SUGGEST_MASK) >> 4;

    if(! driver_max) {
        for(i = 0; driverbyte_table[i]; i++) ;
        driver_max = i;
        for(i = 0; driversuggest_table[i]; i++) ;
        suggest_max = i;
    }
    fprintf(OUTP, "Driver_status=0x%02x",driver_status);
    fprintf(OUTP, " (%s,%s) ",
            dr < driver_max  ? driverbyte_table[dr]:"invalid",
            su < suggest_max ? driversuggest_table[su]:"invalid");
}

#ifdef SG_IO
int sg_chk_n_print3(const char * leadin, struct sg_io_hdr * hp)
{
    return sg_chk_n_print(leadin, hp->masked_status, hp->host_status,
                          hp->driver_status, hp->sbp, hp->sb_len_wr);
}
#endif

int sg_chk_n_print(const char * leadin, int masked_status,
                   int host_status, int driver_status,
                   const unsigned char * sense_buffer, int sb_len)
{
    int done_leadin = 0;
    int done_sense = 0;

    if ((0 == masked_status) && (0 == host_status) &&
        (0 == driver_status))
        return 1;       /* No problems */
    if (0 != masked_status) {
        if (leadin)
            fprintf(OUTP, "%s: ", leadin);
        done_leadin = 1;
        sg_print_status(masked_status);
        fprintf(OUTP, "\n");
        if (sense_buffer && ((masked_status == CHECK_CONDITION) ||
                             (masked_status == COMMAND_TERMINATED))) {
            sg_print_sense(0, sense_buffer, sb_len);
            done_sense = 1;
        }
    }
    if (0 != host_status) {
        if (leadin && (! done_leadin))
            fprintf(OUTP, "%s: ", leadin);
        if (done_leadin)
            fprintf(OUTP, "plus...: ");
        else
            done_leadin = 1;
        sg_print_host_status(host_status);
        fprintf(OUTP, "\n");
    }
    if (0 != driver_status) {
        if (leadin && (! done_leadin))
            fprintf(OUTP, "%s: ", leadin);
        if (done_leadin)
            fprintf(OUTP, "plus...: ");
        else
            done_leadin = 1;
        sg_print_driver_status(driver_status);
        fprintf(OUTP, "\n");
        if (sense_buffer && (! done_sense) &&
            (SG_ERR_DRIVER_SENSE & driver_status))
            sg_print_sense(0, sense_buffer, sb_len);
    }
    return 0;
}

#ifdef SG_IO
int sg_err_category3(struct sg_io_hdr * hp)
{
    return sg_err_category(hp->masked_status, hp->host_status,
                           hp->driver_status, hp->sbp, hp->sb_len_wr);
}
#endif

int sg_err_category(int masked_status, int host_status,
                    int driver_status, const unsigned char * sense_buffer,
                    int sb_len)
{
    if ((0 == masked_status) && (0 == host_status) &&
        (0 == driver_status))
        return SG_ERR_CAT_CLEAN;
    if ((CHECK_CONDITION == masked_status) ||
        (COMMAND_TERMINATED == masked_status) ||
        (SG_ERR_DRIVER_SENSE & driver_status)) {
        if (sense_buffer && (sb_len > 2)) {
            if(RECOVERED_ERROR == sense_buffer[2])
                return SG_ERR_CAT_RECOVERED;
            else if ((UNIT_ATTENTION == (0x0f & sense_buffer[2])) &&
                     (sb_len > 12)) {
                if (0x28 == sense_buffer[12])
                    return SG_ERR_CAT_MEDIA_CHANGED;
                if (0x29 == sense_buffer[12])
                    return SG_ERR_CAT_RESET;
            }
        }
        return SG_ERR_CAT_SENSE;
    }
    if (0 != host_status) {
        if ((SG_ERR_DID_NO_CONNECT == host_status) ||
            (SG_ERR_DID_BUS_BUSY == host_status) ||
            (SG_ERR_DID_TIME_OUT == host_status))
            return SG_ERR_CAT_TIMEOUT;
    }
    if (0 != driver_status) {
        if (SG_ERR_DRIVER_TIMEOUT == driver_status)
            return SG_ERR_CAT_TIMEOUT;
    }
    return SG_ERR_CAT_OTHER;
}

int sg_get_command_size(unsigned char opcode)
{
    return COMMAND_SIZE(opcode);
}
#endif
