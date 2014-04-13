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
 * This file contains the header files for SCSI Generic I/O - copied from LINUX distribution
 */
#if LINUX 
#include <linux/major.h>
#include <sys/sysmacros.h>
#include <ctype.h>
#include <sys/ioctl.h>
#ifdef SG_KERNEL_INCLUDES
  #include "/usr/src/linux/include/scsi/sg.h"
  #include "/usr/src/linux/include/scsi/scsi.h"
#else
  #ifdef SG_TRICK_GNU_INCLUDES
    #include <linux/../scsi/sg.h>
    #include <linux/../scsi/scsi.h>
  #else
    #include <scsi/sg.h>
    #include <scsi/scsi.h>
  #endif
#endif
/*
  Getting the correct include files for the sg interface can be an ordeal.
  In a perfect world, one would just write:
    #include <scsi/sg.h>
    #include <scsi/scsi.h>
  This would include the files found in the /usr/include/scsi directory.
  Those files are maintained with the GNU library which may or may not
  agree with the kernel and version of sg driver that is running. Any
  many cases this will not matter. However in some it might, for example
  glibc 2.1's include files match the sg driver found in the lk 2.2
  series. Hence if glibc 2.1 is used with lk 2.4 then the additional
  sg v3 interface will not be visible.
  If this is a problem then defining SG_KERNEL_INCLUDES will access the
  kernel supplied header files (assuming they are in the normal place).
  The GNU library maintainers and various kernel people don't like
  this approach (but it does work).
  The technique selected by defining SG_TRICK_GNU_INCLUDES worked (and
  was used) prior to glibc 2.2 . Prior to that version /usr/include/linux
  was a symbolic link to /usr/src/linux/include/linux .

  There are other approaches if this include "mixup" causes pain. These
  would involve include files being copied or symbolic links being
  introduced.

  Sorry about the inconvenience. Typically neither SG_KERNEL_INCLUDES
  nor SG_TRICK_GNU_INCLUDES is defined.

  dpg 20010415
*/

#if defined(__GNUC__) || defined(HAS_LONG_LONG)
typedef long long llse_loff_t;
#else
typedef long      llse_loff_t;
#endif

extern llse_loff_t llse_llseek(unsigned int fd,
                               llse_loff_t offset,
			       unsigned int origin);
//--------------------- Previously from sg_err.h ------------------//
#ifndef SG_ERR_H
#define SG_ERR_H

/* Feel free to copy and modify this GPL-ed code into your applications. */

/* Version 0.84 (20010115) 
	- all output now sent to stderr rather thatn stdout
	- remove header files included in this file
*/


/* Some of the following error/status codes are exchanged between the
   various layers of the SCSI sub-system in Linux and should never
   reach the user. They are placed here for completeness. What appears
   here is copied from drivers/scsi/scsi.h which is not visible in
   the user space. */

/* The following are 'host_status' codes */
#ifndef DID_OK
#define DID_OK 0x00
#endif
#ifndef DID_NO_CONNECT
#define DID_NO_CONNECT 0x01     /* Unable to connect before timeout */
#define DID_BUS_BUSY 0x02       /* Bus remain busy until timeout */
#define DID_TIME_OUT 0x03       /* Timed out for some other reason */
#define DID_BAD_TARGET 0x04     /* Bad target (id?) */
#define DID_ABORT 0x05          /* Told to abort for some other reason */
#define DID_PARITY 0x06         /* Parity error (on SCSI bus) */
#define DID_ERROR 0x07          /* Internal error */
#define DID_RESET 0x08          /* Reset by somebody */
#define DID_BAD_INTR 0x09       /* Received an unexpected interrupt */
#define DID_PASSTHROUGH 0x0a    /* Force command past mid-level */
#define DID_SOFT_ERROR 0x0b     /* The low-level driver wants a retry */
#endif

/* These defines are to isolate applictaions from kernel define changes */
#define SG_ERR_DID_OK           DID_OK
#define SG_ERR_DID_NO_CONNECT   DID_NO_CONNECT
#define SG_ERR_DID_BUS_BUSY     DID_BUS_BUSY
#define SG_ERR_DID_TIME_OUT     DID_TIME_OUT
#define SG_ERR_DID_BAD_TARGET   DID_BAD_TARGET
#define SG_ERR_DID_ABORT        DID_ABORT
#define SG_ERR_DID_PARITY       DID_PARITY
#define SG_ERR_DID_ERROR        DID_ERROR
#define SG_ERR_DID_RESET        DID_RESET
#define SG_ERR_DID_BAD_INTR     DID_BAD_INTR
#define SG_ERR_DID_PASSTHROUGH  DID_PASSTHROUGH
#define SG_ERR_DID_SOFT_ERROR   DID_SOFT_ERROR

/* The following are 'driver_status' codes */
#ifndef DRIVER_OK
#define DRIVER_OK 0x00
#endif
#ifndef DRIVER_BUSY
#define DRIVER_BUSY 0x01
#define DRIVER_SOFT 0x02
#define DRIVER_MEDIA 0x03
#define DRIVER_ERROR 0x04
#define DRIVER_INVALID 0x05
#define DRIVER_TIMEOUT 0x06
#define DRIVER_HARD 0x07
#define DRIVER_SENSE 0x08       /* Sense_buffer has been set */

/* Following "suggests" are "or-ed" with one of previous 8 entries */
#define SUGGEST_RETRY 0x10
#define SUGGEST_ABORT 0x20
#define SUGGEST_REMAP 0x30
#define SUGGEST_DIE 0x40
#define SUGGEST_SENSE 0x80
#define SUGGEST_IS_OK 0xff
#endif
#ifndef DRIVER_MASK
#define DRIVER_MASK 0x0f
#endif
#ifndef SUGGEST_MASK
#define SUGGEST_MASK 0xf0
#endif

/* These defines are to isolate applictaions from kernel define changes */
#define SG_ERR_DRIVER_OK        DRIVER_OK
#define SG_ERR_DRIVER_BUSY      DRIVER_BUSY
#define SG_ERR_DRIVER_SOFT      DRIVER_SOFT
#define SG_ERR_DRIVER_MEDIA     DRIVER_MEDIA
#define SG_ERR_DRIVER_ERROR     DRIVER_ERROR
#define SG_ERR_DRIVER_INVALID   DRIVER_INVALID
#define SG_ERR_DRIVER_TIMEOUT   DRIVER_TIMEOUT
#define SG_ERR_DRIVER_HARD      DRIVER_HARD
#define SG_ERR_DRIVER_SENSE     DRIVER_SENSE
#define SG_ERR_SUGGEST_RETRY    SUGGEST_RETRY
#define SG_ERR_SUGGEST_ABORT    SUGGEST_ABORT
#define SG_ERR_SUGGEST_REMAP    SUGGEST_REMAP
#define SG_ERR_SUGGEST_DIE      SUGGEST_DIE
#define SG_ERR_SUGGEST_SENSE    SUGGEST_SENSE
#define SG_ERR_SUGGEST_IS_OK    SUGGEST_IS_OK
#define SG_ERR_DRIVER_MASK      DRIVER_MASK
#define SG_ERR_SUGGEST_MASK     SUGGEST_MASK



/* The following "print" functions send ACSII to stdout */
extern void sg_print_command(const unsigned char * command);
extern void sg_print_sense(const char * leadin,
                           const unsigned char * sense_buffer, int sb_len);
extern void sg_print_status(int masked_status);
extern void sg_print_host_status(int host_status);
extern void sg_print_driver_status(int driver_status);

/* sg_chk_n_print() returns 1 quietly if there are no errors/warnings
   else it prints to standard output and returns 0. */
extern int sg_chk_n_print(const char * leadin, int masked_status,
                          int host_status, int driver_status,
                          const unsigned char * sense_buffer, int sb_len);

/* The following function declaration is for the sg version 3 driver. 
   Only version 3 sg_err.c defines it. */
struct sg_io_hdr;
extern int sg_chk_n_print3(const char * leadin, struct sg_io_hdr * hp);


/* The following "category" function returns one of the following */
#define SG_ERR_CAT_CLEAN 0      /* No errors or other information */
#define SG_ERR_CAT_MEDIA_CHANGED 1 /* interpreted from sense buffer */
#define SG_ERR_CAT_RESET 2      /* interpreted from sense buffer */
#define SG_ERR_CAT_TIMEOUT 3
#define SG_ERR_CAT_RECOVERED 4  /* Successful command after recovered err */
#define SG_ERR_CAT_SENSE 98     /* Something else is in the sense buffer */
#define SG_ERR_CAT_OTHER 99     /* Some other error/warning has occurred */

extern int sg_err_category(int masked_status, int host_status,
               int driver_status, const unsigned char * sense_buffer,
               int sb_len);

/* The following function declaration is for the sg version 3 driver. 
   Only version 3 sg_err.c defines it. */
extern int sg_err_category3(struct sg_io_hdr * hp);

/* Returns length of SCSI command given the opcode (first byte) */
int sg_get_command_size(unsigned char opcode);

#endif
#endif
