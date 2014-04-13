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
 *  Tom Ruwart (tmruwart@ioperformance.com)
 *   I/O Performance, Inc. and
 *   University of Minnesota
 *
 */
#define XDD_VERSION "6.5.013007.0001"
#define XDD_COPYRIGHT "xdd - I/O Performance Inc. Copyright 1992-2007"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef WIN32
#include <io.h>
#include <sys/timeb.h>
#include <time.h>
#include <memory.h>
#include <string.h>
#include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include "mmsystem.h"
#include <Winsock.h>
#else
#include <unistd.h> /* UNIX Only */
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/times.h>
#if !(SOLARIS || HPUX || AIX || OSX)
#include <sys/prctl.h>
#endif
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/resource.h> /* needed for multiple processes */
#include <pthread.h>
#include <sched.h>
#endif
#include <math.h>
#include <sys/stat.h>
#ifdef LINUX
#include <sys/unistd.h>
#include <sys/utsname.h>
#include "sg.h"
#endif
#ifdef SOLARIS
#include <sys/unistd.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <sys/utsname.h>
#endif
#ifdef AIX
#include <sys/processor.h>
#include <ulimit.h>
#include <sys/lock.h>
#include <sys/shm.h>
#include <aio.h>
#include <sys/select.h>
#include <sys/socketvar.h>
#include <sys/utsname.h>
#define _BSD 44
#endif 
#if ( IRIX )
#include <sys/schedctl.h>
#include <sys/lock.h>
#include <sys/sysmp.h> /* needed for multiprocessor stuff */
#include <sys/syssgi.h> /* needed for timestamp table */
#include <sys/immu.h>
#include <ulocks.h>
#include <invent.h>
#include <sys/utsname.h>
#endif
/* for the global clock stuff */
#if (LINUX || AIX || IRIX || SOLARIS || HPUX || ALTIX || OSX)
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif
#if (SNFS)
#include <client/cvdrfile.h>
#endif
#if (HPSS)
#include <hpss_api.h>
#endif
#include "pclk.h" /* pclk_t, prototype compatibility */
#include "misc.h"

#ifdef WIN32 /* ------------------------------ Start of Windows Compatibility Definitions -------------------------------- */
/* This section contains a variety of definintions that map unix-isms to Windows for compatibility */
typedef unsigned    int     uint32_t;
typedef unsigned    long    ulong_t;
typedef             int     int32_t;
typedef unsigned    __int64 uint64_t;
typedef             __int64 int64_t;
typedef unsigned    short   uint16_t;
typedef             short   int16_t;
typedef unsigned    long    in_addr_t; /* An IP number */
typedef unsigned short in_port_t; /* A port number */
typedef SOCKET  sd_t;  /* A socket descriptor */

/* The following structures do not exist in the Windows header files so they have been 
 * recreated here for use with their equivalent functions that live in nt_unix_compat.c as 
 * part of the xdd distribution 
 */
struct tms {
	unsigned long tms_utime;
	unsigned long tms_stime;
};
struct timezone {
	unsigned int tz;
};
struct sched_param {
	unsigned int sched_priority;
};

/* Map pthread structures to their windows equivalents */
struct pthread {
	HANDLE pthread;
};
typedef struct pthread pthread_t;

struct pthread_mutex {
	HANDLE mutex;
	char name[MAX_PATH];
};
typedef struct pthread_mutex pthread_mutex_t;

/* Map semaphore structure to the windows equivalent */
struct sembuf {
	int             sem_op;  /* operation */
	unsigned long   sem_flg; /* flags */
	int             sem_num; /* number */
};

#define SCHED_RR    1   /* Round Robin Scheduling */
#define SCHED_FIFO  2   /* First In First Out Scheduling */
#define MCL_CURRENT 1   /* Lock Current Memory pages  */
#define MCL_FUTURE  2   /* Lock Future memory pages */
#define MP_MUSTRUN  1   /* ASsign this thread to a specific processor */
#define MP_NPROCS   2   /* return the number of processors on the system */
#define IPC_RMID    -3  /* remove  a semaphore */
#define EIDRM       43  /* errno value for Identifer Removed */
#define SETALL 1  /* used for Semaphore ops */
#define IPC_CREAT 01000 /* Used to create Semaphores */

/* These are function calls that do not exist in the Windows CLibrary 
 * so they have been recreated in the nt_unix_compat.c file and their prototypes live here 
 */
int mlockall(unsigned int flag);
int mlock( unsigned char *bp, uint32_t size);
int munlock( unsigned char *bp, uint32_t size);
unsigned long sched_get_priority_max(int flag);
int sched_setscheduler(int arg1, unsigned long arg2, struct sched_param *pp);
int64_t lseek64(unsigned long fd, int64_t offset, int flag);
int fsync(int fd);
int getpagesize(void);
clock_t times( struct tms *tp);
void gettimeofday(struct timeval *tvp, struct timezone *tzp);
void sleep(int seconds);
int sysmp(int op, int arg);
int pthread_create(pthread_t *tp, void *thread_attr, void *thread_function, void *thread_arg);
int32_t pthread_mutex_init(pthread_mutex_t *mp, int n);
int32_t pthread_mutex_destroy(pthread_mutex_t *mp);
void pthread_mutex_lock(pthread_mutex_t *mp);
void pthread_mutex_unlock(pthread_mutex_t *mp);
int semop(HANDLE semid, struct sembuf *sp, int n);
HANDLE semget(unsigned int semid_base, unsigned int maxsem, unsigned int flags);
int semctl(HANDLE semid, unsigned int maxsem, unsigned int flags, unsigned short *zp);
int getpid(void);
int64_t atoll(char *c);
void initstate(int seed, char *state, int value);
int xdd_random_int(void);

#endif /* --------------------------- End of Windows compatibility definitions --------------------------- */

#ifdef HPUX
typedef unsigned short in_port_t;
#endif
#if (LINUX || SOLARIS || HPUX || AIX || OSX)
#define MP_MUSTRUN 1 /* ASsign this thread to a specific processor */
#define MP_NPROCS 2 /* return the number of processors on the system */
typedef int  sd_t;  /* A socket descriptor */
#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif
#endif
#if ( IRIX || ALTIX )
typedef int  sd_t;  /* A socket descriptor */
#endif
/* misc */
#define ONEMEG 1024*1024
#define ON 1
#define OFF 0
#define TRUE 1
#define FALSE 0
#define SUCCESSFUL 1
#define MAX_IDLEN 8192
#define MAX_TARGETS 4096
#define XFS
#define MAXSEM 1
#define MAX_TARGET_NAME_LENGTH 2048
#define HOSTNAMELENGTH 64
/* command line parameter defaults */
#define DEFAULT_OUTPUT "stdout"
#define DEFAULT_ERROUT "stderr"
#define DEFAULT_ID "No ID Specified"
#define DEFAULT_OP "read"
#define DEFAULT_SETUP ""
#define DEFAULT_TARGETDIR ""
#define DEFAULT_TARGET NULL
#define DEFAULT_TIMESTAMP "ts"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_REQSIZE 128
#define DEFAULT_REQINCR 32
#define DEFAULT_NUMREQS 2048
#define DEFAULT_MBYTES 256
#define DEFAULT_PASSES 1
#define DEFAULT_PASSOFFSET 0
#define DEFAULT_PASSDELAY 0
#define DEFAULT_NUM_TARGETS 0
#define DEFAULT_RANGE (1024*1024)
#define DEFAULT_SEED 72058
#define DEFAULT_INTERLEAVE 1
#define DEFAULT_PORT 2000
#define DEFAULT_QUEUEDEPTH 1
#define DEFAULT_BOUNCE 100
#define DEFAULT_NUM_SEEK_HIST_BUCKETS 100
#define DEFAULT_NUM_DIST_HIST_BUCKETS 100
#define DEFAULT_RAW_PORT 2004
#define DEFAULT_RAW_LAG 0
/* program defines */
#define COMMENT '#'
#define DIRECTIVE '@'
#ifndef SPACE
#define SPACE ' '
#endif
#define TAB '\t'
/* Bit settings that are used in the Options 64-bit word */
#define RX_READ               (uint64_t) 0x0000000000000001  /* READ operation */
#define RX_WRITE              (uint64_t) 0x0000000000000002  /* WRITE operation */
#define RX_RAW_READER         (uint64_t) 0x0000000000000004  /* Read-After-Write - reader */
#define RX_RAW_WRITER         (uint64_t) 0x0000000000000008  /* Read-After-Write - writer */
#define RX_HPSS               (uint64_t) 0x0000000000000010  /* target is in HPSS mode */
#define RX_SGIO               (uint64_t) 0x0000000000000020  /* Used for SCSI Generic I/O in Linux */
#define RX_VERIFY_CONTENTS    (uint64_t) 0x0000000000000040  /* Verify the contents of the I/O buffer */
#define RX_VERIFY_LOCATION    (uint64_t) 0x0000000000000080  /* Verify the location of the I/O buffer (first 8 bytes) */
#define RX_DIO                (uint64_t) 0x0000000000000100  /* DIRECT IO for files */
#define RX_TIMER_INFO         (uint64_t) 0x0000000000000200  /* Display Timer information */
#define RX_SYNCIO             (uint64_t) 0x0000000000000400  /* Sync every nth I/O operation */
#define RX_SYNCWRITE          (uint64_t) 0x0000000000000800  /* Sync buffered write operations */
#define RX_RANDOM_PATTERN     (uint64_t) 0x0000000000001000  /* Use random data pattern for write operations */
#define RX_NOBARRIER          (uint64_t) 0x0000000000002000  /* Do not use a barrier */
#define RX_PASSRANDOMIZE      (uint64_t) 0x0000000000004000  /* Pass randomize */
#define RX_MAXPRI             (uint64_t) 0x0000000000008000  /* Maximum process priority */
#define RX_PLOCK              (uint64_t) 0x0000000000010000  /* Lock process in memory */
#define RX_DELETEFILE         (uint64_t) 0x0000000000020000  /* Delete target file upon completion of write */
#define RX_REGULARFILE        (uint64_t) 0x0000000000040000  /* target file is a REGULAR file */
#define RX_DEVICEFILE         (uint64_t) 0x0000000000080000  /* device file I/O */
#define RX_CSV                (uint64_t) 0x0000000000100000  /* Generate Comma Separated Values (.csv) Output file */
#define RX_COMBINED           (uint64_t) 0x0000000000200000  /* Display COMBINED output to a special file */
#define RX_QTHREAD_INFO       (uint64_t) 0x0000000000400000  /* Display Q thread information */
#define RX_WAITFORSTART       (uint64_t) 0x0000000000800000  /* Wait for START trigger */
#define RX_READAFTERWRITE     (uint64_t) 0x0000000001000000  /* Read after write */
#define RX_LOCKSTEP           (uint64_t) 0x0000000002000000  /* Normal Lock step mode */
#define RX_LOCKSTEPOVERLAPPED (uint64_t) 0x0000000004000000  /* Overlapped lock step mode */
#define RX_REALLYVERBOSE      (uint64_t) 0x0000000008000000  /* Really verbose output */
#define RX_DESKEW             (uint64_t) 0x0000000010000000  /* Deskew the performance results */
#define RX_SHARED_MEMORY      (uint64_t) 0x0000000020000000  /* Use a shared memory segment instead of malloced memmory */
#define RX_VERBOSE            (uint64_t) 0x0000000040000000  /* Verbose output */
#define RX_SEQUENCED_PATTERN  (uint64_t) 0x0000000080000000  /* Sequenced Data Pattern in the data buffer */
#define RX_ASCII_PATTERN      (uint64_t) 0x0000000100000000  /* ASCII Data Pattern in the data buffer */
#define RX_HEX_PATTERN        (uint64_t) 0x0000000200000000  /* HEXIDECIMAL Data Pattern in the data buffer */
#define RX_SINGLECHAR_PATTERN (uint64_t) 0x0000000400000000  /* HEXIDECIMAL Data Pattern in the data buffer */
#define RX_FILE_PATTERN       (uint64_t) 0x0000000800000000  /* Name of file that contains the Data Pattern */
#define RX_REPLICATE_PATTERN  (uint64_t) 0x0000001000000000  /* Replicate Data Pattern throughout the data buffer */
#define RX_NOMEMLOCK          (uint64_t) 0x0000002000000000  /* Do not lock memory */
#define RX_NOPROCLOCK         (uint64_t) 0x0000004000000000  /* Do not lock process */
#define RX_REOPEN             (uint64_t) 0x0000008000000000  /* Open/Close target on each pass and record time */
#define RX_CREATE_NEW_FILES   (uint64_t) 0x0000010000000000  /* Create new targets for each pass */
#define RX_RECREATE           (uint64_t) 0x0000020000000000  /* ReCreate targets for each pass */

/* ts_options bit settings */
#define TS_NORMALIZE          0x00000001 /* Time stamping normalization of output*/
#define TS_ON                 0x00000002 /* Time stamping is ON */
#define TS_SUMMARY            0x00000004 /* Time stamping Summary reporting */
#define TS_DETAILED           0x00000008 /* Time stamping Detailed reporting */
#define TS_APPEND             0x00000010 /* Time stamping Detailed reporting */
#define TS_DUMP               0x00000020 /* Time stamping Dumping */
#define TS_WRAP               0x00000040 /* Wrap the time stamp buffer */
#define TS_ONESHOT            0x00000080 /* Stop time stamping at the end of the buffer */
#define TS_STOP               0x00000100 /* Stop time stamping  */
#define TS_ALL                0x00000200 /* Time stamp all operations */
#define TS_TRIGTIME           0x00000400 /* Time stamp trigger time */
#define TS_TRIGOP             0x00000800 /* Time stamp trigger operation number */
#define TS_TRIGGERED          0x00001000 /* Time stamping has been triggered */
#define TS_SUPPRESS_OUTPUT    0x00002000 /* Suppress timestamp output */

#ifdef LINUX_NETSTUFF /* Needed for Linux only */
typedef unsigned long in_addr_t;  /* An IP number */
typedef unsigned short in_port_t;  /* A port number */    
#endif
/*
 * Note that IP addresses are stored in host byte order; they're
 * converted to network byte order prior to use for network routines.
 */
typedef struct flags_t {
	bool fl_server; /* true: runs as server; false: run as client */
	in_addr_t fl_addr; /* IP address to use */
	in_port_t fl_port; /* Port number to use */
	uint32_t fl_count; /* Number of time bounces */
	pclk_t fl_time; /* Global clock time to wait for */
} flags_t;
/* --------- */
/* Constants */
/* --------- */
#define BACKLOG SOMAXCONN /* Pending request limit for listen() */
/* XXX *//* This needs to be converted to use a config file */
/* Default flag values */
#define DFL_FL_SERVER false  /* Client by default */
#if (LINUX || AIX || IRIX || SOLARIS || HPUX || ALTIX)
#define DFL_FL_ADDR INADDR_ANY /* Any address */  /* server only */
#else /* Windows */
#define DFL_FL_ADDR 0x8065b61b /*  128.101.182.27 */
#endif
#define DFL_FL_PORT 2000  /* Port to use */
#define DFL_FL_COUNT 10  /* Bounce a hundred times */
#if (LINUX || AIX || IRIX || SOLARIS || HPUX || ALTIX || OSX)
#define DFL_FL_TIME 99160##000000000000LL /* Zero means don't wait */
#else
#define DFL_FL_TIME 99160##000000000000I64 /* Zero means don't wait */
#endif
/* XXX *//* Windows might be limiting the range from 1024 to 5000 */
#define PORT_MAX USHRT_MAX /* Max value for a port number */
/* --------------- */
/* Private globals */
/* --------------- */
static flags_t flags = { /* Options specified on the command line */
	DFL_FL_SERVER,
	DFL_FL_ADDR,
	DFL_FL_PORT,
	DFL_FL_COUNT,
	DFL_FL_TIME
};
/* ------ */
/* Macros */
/* ------ */
#ifdef WIN32  /* Windows environment */
/*
 * We use this for the first argument to select() to avoid having it
 * try to process all possible socket descriptors. Windows ignores
 * the first argument to select(), so we could use anything.  But we
 * use the value elsewhere so we give it its maximal value.
 */
#define getdtablehi() FD_SETSIZE  /* Number of sockets in a fd_set */
#else /* UNIX in general */                  
/* Windows distinguishes between sockets and files; UNIX does not. */
#define closesocket(sd) close(sd)
static bool  sockets_init(void);
#endif /* WINDOWS_NT */
/* This is the time stamping Trace Table Entry structure.
 * There is one of these entries for each I/O operation performed
 * during a test.
 */
/* typedef unsigned long long iotimer_t; */
struct tte {
	short pass;  /* pass number */
	char rwvop;  /* operation: write=2, read=1 */
	char filler1; /* */
	int32_t opnumber; /* operation number */
	uint64_t byte_location; /* seek location in bytes */
	pclk_t start;  /* The starting time stamp */
	pclk_t end;  /* The ending time stamp */
};
typedef struct tte tte_t;
/* Time stamp Trace Table Header - this gets written out before
 * the time stamp trace table data 
 */
struct tthdr {
	int32_t reqsize; /* size of these requests in 'blocksize'-byte blocks */
	int32_t blocksize; /* size of each block in bytes */
	int32_t numents; /* number of timestamp table entries */
	pclk_t trigtime; /* Time the time stamp started */
	int32_t trigop;  /* Operation number that timestamping started */
	int64_t res;  /* clock resolution - pico seconds per clock tick */
	int64_t range;  /* range over which the IO took place */
	int64_t start_offset; /* offset of the starting block */
	int64_t target_offset; /* offset of the starting block for each proc*/
	uint64_t global_options;  /* options used */
	uint64_t target_options;  /* options used */
	char id[MAX_IDLEN]; /* ID string */
	char td[32];  /* time and date */
	pclk_t timer_oh; /* Timer overhead in nanoseconds */
	pclk_t delta;  /* Delta used for normalization */
	int32_t tt_bytes; /* Size of the entire time stamp table in bytes */
	int32_t tt_size; /* Size of the entire time stamp table in entries */
	int64_t tte_indx; /* Index into the time stamp table */
	struct tte tte[1]; /* timestamp table entries */
};
typedef struct tthdr tthdr_t;
/* The seek header contains all the information regarding seek locations */
struct seek_entries {
	int32_t operation; /* read or write */
	int32_t reqsize; /* Size of data transfer in blocks */
	uint64_t block_location; /* Starting location in blocks */
	pclk_t time1;  /* Relative time in pico seconds that this operation should start */
	pclk_t time2;  /* not yet implemented */
};
typedef struct seek_entries seek_t;
/* Bit settings used for the vairous seek options */
#define RX_SEEK_SAVE  0x00000001 /* Save seek locations in a file */
#define RX_SEEK_LOAD  0x00000002 /* Load seek locations from a file */
#define RX_SEEK_RANDOM  0x00000004 /* Random seek locations */
#define RX_SEEK_STAGGER  0x00000008 /* Staggered seek locations */
#define RX_SEEK_NONE  0x00000010 /* No seek locations */
#define RX_SEEK_DISTHIST 0x00000020 /* Print the seek distance histogram */
#define RX_SEEK_SEEKHIST 0x00000040 /* Print the seek location histogram */
struct seekhdr {
	uint64_t seek_options; /* various seek option flags */
	int64_t  seek_range; /* range of seek locations */
	int32_t  seek_seed; /* seed used for generating random seek locations */
	int32_t  seek_interleave; /* interleave used for generating sequential seek locations */
	uint32_t seek_iosize; /* The largest I/O size in the list */
	int32_t  seek_num_rw_ops;  /* Number of read+write operations */
	int32_t  seek_total_ops;   /* Total number of ops in the seek list including verifies */
	int32_t  seek_NumSeekHistBuckets;/* Number of buckets for seek histogram */
	int32_t  seek_NumDistHistBuckets;/* Number of buckets for distance histogram */
	char  *seek_savefile; /* file to save seek locations into */
	char  *seek_loadfile; /* file from which to load seek locations from */
	char  *seek_pattern; /* The seek pattern used for this target */
	seek_t  *seeks;  /* the seek list */
};
typedef struct seekhdr seekhdr_t; 
/* These barriers are used for thread synchronization */
struct xdd_barrier {
	struct xdd_barrier *prev; /* Previous barrier in the chain */
	struct xdd_barrier *next; /* Next barrier in chain */
	int32_t  initialized; /* TRUE means this barrier has been initialized */
	char  name[256];
	int32_t  counter; /* counter variable */
	int32_t  threads; /* number of threads participating */
	int32_t  semid_base; /* Semaphore ID base */
#ifdef WIN32
	HANDLE  semid;  /* id for this sempahore */
#else
	int32_t  semid;  /* id for this semaphore */
#endif
	pthread_mutex_t mutex;  /* locking mutex */
};
typedef struct xdd_barrier xdd_barrier_t; 
/* results struction for multiple processes */
struct results {
	int32_t  errors;  /* Number of I/O errors */
	int64_t  reqsize; /* RequestSize in bytes */
	int64_t  passes_completed; /* Number of passes completed so far */
	char    optype[16]; /* operation type - read, write, or mixed */
	double  avg_rate; /* Average data rate in 10^6 bytes/sec */
	double  hi_rate; /* Highest data rate in 10^6 bytes/sec */
	double  low_rate; /* Lowest data rate in 10^6 bytes/sec */
	double  deskewed_rate; /* The deskewed data rate (valid when the -deskew option is used) */
	double  hi_elapsed; /* Highest elapsed time in seconds */
	double  low_elapsed; /* Lowest elapsed time in seconds */
	double  elapsed; /* Elapsed time in seconds */
	double  deskew_window_time; /* De-skew window time in seconds */
	double  frontend_skew_time; /* Front end skew time in seconds */
	double  backend_skew_time; /* Back end skew time in seconds */
	double  avg_latency; /* Average latency time in seconds */
	double  hi_latency; /* Highest latency time in seconds */
	double  low_latency; /* Lowest latency time in seconds */
	int64_t  bytes;  /* bytes transfered */
	int64_t  deskew_window_bytes; /* bytes transfered during deskew window*/
	int64_t  frontend_skew_bytes; /* bytes transfered during front end skew period */
	int64_t  backend_skew_bytes;  /* bytes transfered during back  end skew period */
	int64_t  ops;    /* operations performed */
	int64_t  deskewed_ops;  /* operations performed during deskew period */
	int64_t  compare_errors; /* Number of compare errors on a sequenced data pattern check */
	double  avg_iops; /* Average number of operations per second */
	double  deskewed_iops; /* Average number of operations per second during deskew period */
	double  user_time; /* Amount of CPU time used by the application */
	double  system_time; /* Amount of CPU time used by the system */
	double  us_time; /* Total CPU time used by this process: user+system time */
	double  wall_clock_time;/* Wall clock time for this pass */
	double  percent_cpu; /* Percent of CPU used by this process */
	pclk_t  start_time; /* Start time for this pass */
	pclk_t  end_time; /* End time for this pass */
	pclk_t  accum_time; /* acumulated time during a run */
	pclk_t  earliest_start_time; /* Used to determine overall actual I/O time */
	pclk_t  latest_end_time; /* Used to determine overall actual I/O time */
	pclk_t  earliest_end_time; /* Used to determine back-end skew */
	pclk_t  latest_start_time; /* Used to determine front-end skew */
	pclk_t  open_start_time; /* Time stamp at the beginning of openning the target */
	pclk_t  open_end_time; /* Time stamp after openning the target */
	pclk_t  open_time; /* Time spent openning the target: end_time-start_time*/
}; 
typedef struct results results_t;
/* results struction for multiple processes */
struct xdd_profile {
	pclk_t  start_time; /* Start time for this pass */
	pclk_t  end_time; /* End time for this pass */
	pclk_t  init_time; /* acumulated time during a run */
	pclk_t  earliest_start_time;
	pclk_t  latest_end_time;
}; 
typedef struct xdd_profile xdd_profile_t;
/* results structure for read-after-write processes */
struct xdd_raw_msg {
	uint32_t magic;  /* Magic number */
	int32_t  sequence; /* Sequence number */
	pclk_t  sendtime; /* Time this packet was sent in global pico seconds */
	pclk_t  recvtime; /* Time this packet was received in global pico seconds */
	int64_t  location; /* Starting location in bytes for this operation */
	int64_t  length;  /* Length in bytes this operation */
}; 
typedef struct xdd_raw_msg xdd_raw_msg_t;
/* Timing Profile structure */
struct xdd_timing_profile {
	pclk_t        base_time;  /* This is time t0 and is the same for all targets and all queue threads */
	pclk_t        open_start_time; /* Time just before the open is issued for this target */
	pclk_t        open_end_time; /* Time just after the open completes for this target */
	pclk_t        my_start_time; /* This is the time stamp just before the first actual I/O operation for this T/Q thread */
	pclk_t        my_current_start_time; /* This is the time stamp of the current I/O Operation for thsi T/Q Thread */
	pclk_t        my_end_time; /* This is the time stamp just after the last I/O operation performed by this T/Q thread */
	pclk_t        my_current_end_time; /* This is the time stamp of the current I/O Operation for thsi T/Q Thread */
	pclk_t        my_current_elapsed_time; /* This is the time stamp of the current I/O Operation for thsi T/Q Thread */
}; 
typedef struct xdd_timing_profile xdd_timing_profile_t;

/* Per Thread Data Structure - one for each thread */
struct ptds {
	struct   ptds *nextp; /* pointers to the next ptds in the queue */
	pthread_t  thread;   /* Handle for this thread */
	int32_t   mynum;   /* my target number */
	int32_t   myqnum;   /* My queue number within this target */
	int32_t   mythreadnum; /* My thread number */
	int32_t   mythreadid;  /* My system thread ID (like a process ID) */
	int32_t   mypid;   /* My process ID */
	int32_t   thread_complete; /* 0 = thread has not completed yet, 1= thread is done */
	int32_t   total_threads; /* Total number of threads -> target threads + QThreads */
#ifdef WIN32
	HANDLE   fd;    /* File HANDLE for the target device/file */
#else
	int32_t   fd;    /* File Descriptor for the target device/file */
#endif
	unsigned char *rwbuf;   /* the re-aligned I/O buffers */
	int32_t       rwbuf_shmid; /* Shared Memeory ID for rwbuf */
	unsigned char *rwbuf_save; /* the original I/O buffers */
	tthdr_t       *ttp;   /* pointers to the timestamp table */
	int32_t       iobuffersize; /* size of the I/O buffer in bytes */
	int32_t       iosize;   /* number of bytes per request */
	int32_t       actual_iosize;   /* number of bytes actually transferred for this request */
	int32_t       last_iosize; /* number of bytes for the final request */
	int32_t       current_pass; /* Current pass number */
	int32_t       filetype;  /* Type of file: regular, device, socket, ... */
	int64_t       filesize;  /* size of target file in bytes */
	int64_t       total_ops;  /* total number of ops to perform */
	int64_t       residual_ops;  /* number of requests mod the queue depth */
	int64_t       writer_total; /* Total number of bytes written so far - used by read after write*/
	seekhdr_t     seekhdr;  /* For all the seek information */
	results_t     perpass;  /* Intermediate Results for this pass for this qthread */
	results_t     qthread_results;/* Cumulative results for this qthread */
	results_t     target_results; /* Cumulative results for this target <only used by qthread 0> */
	FILE          *tsfp;   /* Pointer to the time stamp output file */
#ifdef WIN32
	HANDLE        *ts_serializer_mutex; /* needed to circumvent a Windows bug */
	char          ts_serializer_mutex_name[256]; /* needed to circumvent a Windows bug */
#endif
	/* command line option values */
	uint64_t      start_offset; /* starting block offset value */
	int64_t       pass_offset; /* number of blocks to add to seek locations between passes */
	int32_t       kbytes;   /* number of 1024-byte blocks to process overall */
	int32_t       mbytes;   /* number of 1024x1024-byte blocks to process overall */
	uint64_t      numreqs;  /* number of requests to perform per pass */
	double        rwratio;  /* read/write ratios */
	pclk_t        report_threshold;  /* reporting threshold for long operations */
	int32_t       reqsize;  /* number of *blocksize* byte blocks per operation for each target */
	int32_t       time_limit;  /* timelimit in seconds for each thread */
	char          *targetdir;  /* The target directory for the target */
	char          *target;  /* devices to perform operation on */
	char          targetext[32]; /* The target extension number */
	int32_t       processor;  /* Processor/target assignments */
	pclk_t        start_delay; /* number of picoseconds to delay the start  of this operation */
	double        throttle;  /* Target Throttle assignments */
	double        throttle_variance;  /* Throttle Average Bandwidth variance */
#define RX_THROTTLE_OPS   0x00000001  /* Throttle type of OPS */
#define RX_THROTTLE_BW    0x00000002  /* Throttle type of Bandwidth */
#define RX_THROTTLE_ABW   0x00000004  /* Throttle type of Average Bandwidth */
	uint32_t      throttle_type; /* Target Throttle type */
	char          raw_myhostname[HOSTNAMELENGTH]; /* Hostname of the reader machine as seen by the reader */
	char          *raw_hostname; /* Name of the host doing the reading in a read-after-write */
	struct hostent *raw_hostent; /* for the reader/writer host information */
	in_port_t     raw_port;  /* Port number to use for the read-after-write socket */
	in_addr_t     raw_addr;  /* Address number of the read-after-write socket */
	int32_t       raw_lag;  /* Number of blocks the reader should lag behind the writer */
#define RX_RAW_STAT 0x00000001  /* Read-after-write should use stat() to trigger read operations */
#define RX_RAW_MP   0x00000002  /* Read-after-write should use message passing from the writer to trigger read operations */
	uint32_t      raw_trigger; /* Read-After-Write trigger mechanism */
	int32_t       raw_sd;   /* Socket descriptor for the read-after-write message port */
	int32_t       raw_nd;   /* Number of Socket descriptors in the read set */
	sd_t          raw_csd[FD_SETSIZE];/* Client socket descriptors */
	fd_set        raw_active;  /* This set contains the sockets currently active */
	fd_set        raw_readset; /* This set is passed to select() */
	struct sockaddr_in  raw_sname; /* used by setup_server_socket */
	uint32_t      raw_snamelen; /* the length of the socket name */
	int32_t       raw_current_csd; /* the current csd used by the raw reader select call */
	int32_t       raw_next_csd; /* The next available csd to use */
#define RX_RAW_MAGIC 0x07201958 /* The magic number that should appear at the beginning of each message */
	xdd_raw_msg_t raw_msg;  /* The message sent in from the writer */
	int32_t       raw_msg_sequence; /* The last raw msg sequence number received */
	int32_t       raw_msg_sent; /* The number of messages sent */
	int32_t       raw_msg_recv; /* The number of messages received */
	int32_t       timestamps;  /* The number of times a time stamp was taken */
	uint64_t      ts_options;  /* Time Stamping Options */
	uint32_t      ts_size;  /* Time Stamping Size in number of entries */
	pclk_t        ts_trigtime; /* Time Stamping trigger time */
	int32_t       ts_trigop;  /* Time Stamping trigger operation number */
	volatile int32_t pass_ring;  /* what xdd hears when the time limit is exceeded for a single pass */
	uint64_t      bytestoxfer; /* number of bytes to xfer per pass */
	int32_t       block_size;  /* size of a block in bytes for this target */
	int32_t       queue_depth; /* command queue depth for each target */
	unsigned char *data_pattern; /* data pattern for write operations for each target */
	unsigned char *data_pattern_buffer; /* A buffer up to the size of the IO buffer that contains the data pattern to write or compare against */
	int32_t       data_pattern_length; /* length of ASCII data pattern for write operations for each target */
	char          *data_pattern_filename; /* name of a file that contains a data pattern to use */
	int32_t       preallocate; /* file preallocation value */
	int32_t       align;   /* Memory read/write buffer alignment value in bytes */
	uint64_t      target_options; /* I/O Options specific to each target */
	pclk_t        base_time;  /* This is time t0 and is the same for all targets and all queue threads */
	pclk_t        open_start_time; /* Time just before the open is issued for this target */
	pclk_t        open_end_time; /* Time just after the open completes for this target */
	pclk_t        my_start_time; /* This is the time stamp just before the first actual I/O operation for this T/Q thread */
	pclk_t        my_current_start_time; /* This is the time stamp of the current I/O Operation for this T/Q Thread */
	pclk_t        my_end_time; /* This is the time stamp just after the last I/O operation performed by this T/Q thread */
	pclk_t        my_current_end_time; /* This is the time stamp of the current I/O Operation for thsi T/Q Thread */
	pclk_t        my_current_elapsed_time; /* This is the time stamp of the current I/O Operation for thsi T/Q Thread */
	uint64_t      my_current_byte_location; /* Current byte location for this I/O operation */
	uint64_t      front_end_skew_bytes; /* The number of bytes transfered during the front end skew period */
	uint64_t      deskew_window_bytes; /* The number of bytes transferred during the deskew window */
	uint64_t      back_end_skew_bytes; /* The number of bytes transfered during the back end skew period */
	/* The following variables are used to implement the various trigger options */
	pclk_t        start_trigger_time; /* Time to trigger another target to start */
	pclk_t        stop_trigger_time; /* Time to trigger another target to stop */
	int64_t       start_trigger_op; /* Operation number to trigger another target to start */
	int64_t       stop_trigger_op; /* Operation number  to trigger another target to stop */
	double        start_trigger_percent; /* Percentage of ops before triggering another target to start */
	double        stop_trigger_percent; /* Percentage of ops before triggering another target to stop */
	int64_t       start_trigger_bytes; /* Number of bytes to transfer before triggering another target to start */
	int64_t       stop_trigger_bytes; /* Number of bytes to transfer before triggering another target to stop */
#define TRIGGER_STARTTIME    0x00000001 /* Trigger type of "time" */
#define TRIGGER_STARTOP      0x00000002 /* Trigger type of "op" */
#define TRIGGER_STARTPERCENT 0x00000004 /* Trigger type of "percent" */
#define TRIGGER_STARTBYTES   0x00000008 /* Trigger type of "bytes" */
	uint32_t      trigger_types;  /* This is the type of trigger to administer to another target */
	int32_t       start_trigger_target; /* The number of the target to send the start trigger to */
	int32_t       stop_trigger_target; /* The number of the target to send the stop trigger to */
	uint32_t      run_status;   /* This is the status of this thread 0=not started, 1=running */
	xdd_barrier_t Start_Trigger_Barrier[2]; /* Start Trigger Barrier */
	int32_t       Start_Trigger_Barrier_index; /* The index for the Start Trigger Barrier */
	/* The following variables are used to implement the lockstep options */
	pthread_mutex_t ls_mutex;  /* This is the lock-step mutex used by this target */
	int32_t       ls_task_counter; /* This is the number of times that the master has requested this 
									  * slave to perform a task. 
									  * Each time the master needs this slave to perform a task this counter is
									  * incremented by 1.
									  * Each time this slave completes a task, this counter is decremented by 1.
									  * Access to this counter is protected by the ls_mutex. 
									  */
#define LS_INTERVAL_TIME  0x00000001 /* Task type of "time" */
#define LS_INTERVAL_OP   0x00000002 /* Task type of "op" */
#define LS_INTERVAL_PERCENT  0x00000004 /* Task type of "percent" */
#define LS_INTERVAL_BYTES  0x00000008 /* Task type of "bytes" */
	uint32_t  ls_interval_type; /* Flags used by the lock-step master */
	char   *ls_interval_units; /* ASCII readable units for the interval value */
	int64_t   ls_interval_value; /* This is the value of the interval on which the lock step occurs */
	uint64_t  ls_interval_base_value;  /* This is the base value to which the interval value is compared to */
#define LS_TASK_TIME  0x00000001 /* Task type of "time" */
#define LS_TASK_OP   0x00000002 /* Task type of "op" */
#define LS_TASK_PERCENT  0x00000004 /* Task type of "percent" */
#define LS_TASK_BYTES  0x00000008 /* Task type of "bytes" */
	uint32_t  ls_task_type; /* Flags used by the lock-step master */
	char   *ls_task_units; /* ASCII readable units for the task value */
	int64_t   ls_task_value; /* Depending on the task type (time, ops, or bytes) this variable will be
									 * set to the appropriate number of seconds, ops, or bytes to run/execute/transfer
									 * per "task".
									 */
	uint64_t  ls_task_base_value;  /* This is the base value to which the task value is compared to */
#define LS_SLAVE_WAITING   0x00000001 /* The slave is waiting for the master to enter the ls_barrier */
#define LS_SLAVE_RUN_IMMEDIATELY 0x00000002 /* The slave should start running immediately */
#define LS_SLAVE_COMPLETE   0x00000004 /* The slave should complete all operations after this I/O */
#define LS_SLAVE_STOP    0x00000008 /* The slave should abort after this I/O */
#define LS_SLAVE_FINISHED   0x00000010 /* The slave is finished */
#define LS_MASTER_FINISHED   0x00000020 /* The master has completed its pass */
#define LS_MASTER_WAITING   0x00000040 /* The master is waiting at the barrier */
	uint32_t  ls_ms_state; /* This is the state of the master and slave at any given time. If this is set to SLAVE_WAITING
									 * then the slave has entered the ls_barrier and is waiting for the master to enter
									 * so that it can continue. This is checked by the master so that it will enter only
									 * if the slave is there waiting. This prevents the master from being blocked when
									 * doing overlapped-lock-step operations.
									 */
	int32_t   ls_master;  /* The target number that is the master if this target is a slave */
	int32_t   ls_slave;  /* The target number that is the slave if this target is the master */ 
	xdd_barrier_t Lock_Step_Barrier[2]; /* Lock Step Barrier for non-overlapped lock stepping */
	int32_t   Lock_Step_Barrier_Master_Index; /* The index for the Lock Step Barrier */
	int32_t   Lock_Step_Barrier_Slave_Index; /* The index for the Lock Step Barrier */
	struct ptds  *pm1;   /* ptds minus  1 - used for report print queueing - don't ask */
};
typedef struct ptds ptds_t;
/* function prototypes */
void xdd_alarm(void);
int32_t xdd_atohex(char *destp, char *sourcep);
int32_t xdd_barrier(struct xdd_barrier *bp);
unsigned char *xdd_init_io_buffers(ptds_t *p);
void xdd_calculate_rates(results_t *results);
void xdd_combine_results(results_t *from, results_t *to);
void xdd_config_info(void);
int32_t xdd_deskew(void);
void xdd_destroy_all_barriers(void);
void xdd_destroy_barrier(struct xdd_barrier *bp);
void xdd_display_combined_results(void);
void xdd_display_qthread_pass_results(ptds_t *p, FILE *ofp, FILE *cfp, int32_t verbosity);
void xdd_display_qthread_average_results(ptds_t *p, FILE *ofp, FILE *cfp, int32_t verbosity);
void xdd_display_results(char *id, int32_t target, int32_t queue, int64_t bytes, int64_t ops, double time, double rate, double iops, double latency, double percentcpu, char *optype, int64_t reqsize, pclk_t open_time, pclk_t start_time, pclk_t end_time, FILE *ofp, FILE *cfp, uint64_t verbosity);
void xdd_display_results_header(void);
void xdd_display_csv_results(char *id, int32_t target, int32_t queue, int64_t bytes, int64_t ops, double time, double rate, double iops, double latency, double percentcpu, char *optype, int64_t reqsize, pclk_t open_time, pclk_t start_time, pclk_t end_time, FILE *cfp, uint64_t verbosity);
void xdd_display_target_pass_results(ptds_t *p, FILE *ofp, FILE *cfp, int32_t verbosity);
void xdd_display_target_average_results(ptds_t *p, FILE *ofp, FILE *cfp, int32_t verbosity);
void xdd_display_system_info(void);
void xdd_do_io(char *target, int mynum);
void xdd_init_seek_list(ptds_t *p);
char *xdd_getnexttoken(char *tp);
void* xdd_heartbeat(void*);
void* xdd_hpss_mover_manager(void *junk);
void* xdd_hpss_control(void *junk);
void xdd_init_all_barriers(void);
int32_t xdd_init_barrier(struct xdd_barrier *bp, int32_t threads, char *barrier_name);
void xdd_init_global_clock(pclk_t *pclkp); 
in_addr_t xdd_init_global_clock_network(char *hostname);
void xdd_init_globals(void);
void xdd_init_ptds(ptds_t *p, int32_t n);
void xdd_init_results(results_t *results);
void xdd_init_signals(void);
int32_t xdd_io_loop(ptds_t *p, results_t *results);
void* xdd_io_thread(void *p); 
int32_t xdd_linux_cpu_count(void); 
int32_t xdd_load_seek_list(ptds_t *p);
void xdd_lock_memory(unsigned char *bp, uint32_t bsize, char *sp);
#ifdef WIN32
HANDLE xdd_open_target(ptds_t *p);
#else
int32_t xdd_open_target(ptds_t *p);
#endif
void xdd_options_info(FILE *out);
void xdd_pattern_buffer(ptds_t *p);
void xdd_parse(int argc, char *argv[]);
void xdd_parse_args(int argc,char *argv[]);
void xdd_processor(ptds_t *p);
int32_t xdd_process_directive(char *lp);
int32_t xdd_process_paramfile(char *fnp);
double xdd_random_float(void);
void xdd_raw_err(char const *fmt, ...);
int32_t xdd_raw_read_wait(ptds_t *p); 
int32_t xdd_raw_reader_init(ptds_t *p);
int32_t xdd_raw_setup_reader_socket(ptds_t *p);
int32_t xdd_raw_setup_writer_socket(ptds_t *p);
int32_t xdd_raw_sockets_init(void);
int32_t xdd_raw_writer_init(ptds_t *p);
int32_t xdd_raw_writer_send_msg(ptds_t *p);
void xdd_save_seek_list(ptds_t *p);
void xdd_schedule_options(void);
void xdd_set_timelimit(void);
void xdd_system_info(FILE *out);
void xdd_target_info(FILE *out, ptds_t *p);
void xdd_ts_cleanup(struct tthdr *ttp);
void xdd_ts_overhead(struct tthdr *ttp);
void xdd_ts_reports(ptds_t *p);
void xdd_ts_setup(ptds_t *p);
void xdd_ts_write(ptds_t *p);
void xdd_unlock_memory(unsigned char *bp, uint32_t bsize, char *sp);
void xdd_usage(int fullhelp);
int32_t xdd_verify(ptds_t *p, int32_t current_op);
int32_t xdd_verify_location(ptds_t *p, int32_t current_op);
int32_t xdd_verify_contents(ptds_t *p, int32_t current_op);
int32_t xdd_verify_checksum(ptds_t *p, int32_t current_op);
int32_t xdd_verify_hex(ptds_t *p, int32_t current_op);
int32_t xdd_verify_sequence(ptds_t *p, int32_t current_op);
int32_t xdd_verify_singlechar(ptds_t *p, int32_t current_op);
extern int errno;
void clk_delta(in_addr_t addr, in_port_t port, int32_t bounce, pclk_t *pclkp);
void clk_initialize(in_addr_t addr, in_port_t port, int32_t bounce, pclk_t *pclkp);
#ifdef WIN32
int32_t ts_serializer_init(HANDLE *mp, char *mutex_name);
#endif

struct xdd_globals {
/* Global variables relevant to all threads */
     uint64_t     global_options;         /* I/O Options valid for all targets */
     volatile     int32_t canceled;       /* Normally set to 0. Set to 1 by xdd_sigint when an interrupt occurs */
     char         id_firsttime;           /* ID first time through flag */
     volatile     int32_t run_ring;       /* The alarm that goes off when the total run time has been exceeded */
     volatile     int32_t deskew_ring;    /* The alarm that goes off when the the first thread finishes */
     volatile     int32_t abort_io;       /* abort the run due to some catastrophic failure */
     time_t       current_time_for_this_run; /* run time for this run */
     char         *progname;              /* Program name from argv[0] */
     int32_t      argc;                   /* The original arg count */
     char         **argv;                 /* The original *argv[]  */
     int32_t      passes;                 /* number of passes to perform */
     int32_t      passdelay;              /* number of seconds to delay between passes */
     int64_t      max_errors;             /* max number of errors to tollerate */
     char         *output_filename;       /* name of the output file */
     char         *errout_filename;       /* name fo the error output file */
     char         *csvoutput_filename;    /* name of the csv output file */
     char         *combined_output_filename; /* name of the combined output file */
     char         *tsbinary_filename;     /* timestamp filename prefix */
     char         *tsoutput_filename;     /* timestamp report output filename prefix */
     FILE         *output;                /* Output file pointer*/ 
     FILE         *errout;                /* Error Output file pointer*/ 
     FILE         *csvoutput;             /* Comma Separated Values output file */
     FILE         *combined_output;       /* Combined output file */
     uint32_t     heartbeat;              /* seconds between heartbeats */
     int32_t      syncio;                 /* the number of I/Os to perform btw syncs */
     uint64_t     target_offset;          /* offset value */
     int32_t      number_of_targets;      /* number of targets to operate on */
     int32_t      number_of_iothreads;    /* number of threads spawned for all targets */
     char         *id;                    /* ID string pointer */
     int32_t      runtime;                /* Length of time to run all targets, all passes */
     pclk_t       estimated_end_time;     /* The time at which this run (all passes) should end */
     int32_t      number_of_processors;   /* Number of processors */
     char         random_init_state[256]; /* Random number generator state initalizer array */ 
     int          random_initialized;     /* Random number generator has been initialized */
/* information needed to access the Global Time Server */
     in_addr_t    gts_addr;               /* Clock Server IP address */
     in_port_t    gts_port;               /* Clock Server Port number */
     pclk_t       gts_time;               /* global time on which to sync */
     pclk_t       gts_seconds_before_starting; /* number of seconds before starting I/O */
     int32_t      gts_bounce;             /* number of times to bounce the time off the global time server */
     pclk_t       gts_delta;              /* Time difference returned by the clock initializer */
     char         *gts_hostname;          /* name of the time server */
     pclk_t       ActualLocalStartTime;   /* The time to start operations */
/* Thread Barriers */
     pthread_mutex_t  xdd_init_barrier_mutex; /* locking mutex for the xdd_init_barrier() routine */
     xdd_barrier_t    *barrier_chain;         /* First barrier on the chain */
     int32_t          barrier_count;          /* Number of barriers on the chain */
     xdd_barrier_t    thread_barrier[2];      /* barriers for synchronization */
     xdd_barrier_t    syncio_barrier[2];      /* barriers for syncio */
     xdd_barrier_t    serializer_barrier[2];  /* barriers for serialization of pthread_create() */
     xdd_barrier_t    prefinal_barrier;       /* barrier for synchronization */
     xdd_barrier_t    final_barrier;          /* barrier for synchronization */
     xdd_barrier_t    QThread_Barrier[MAX_TARGETS][2];
#ifdef WIN32
     HANDLE       ts_serializer_mutex;        /* needed to circumvent a Windows bug */
     char         *ts_serializer_mutex_name;  /* needed to circumvent a Windows bug */
#endif

/* teporary until DESKEW results are fixed */
double deskew_total_rates;
double deskew_total_time;
uint64_t deskew_total_bytes;

/* Target Specific variables */
     ptds_t  ptds[MAX_TARGETS];     /* Per Target Data Structures */
};
typedef struct xdd_globals xdd_globals_t;

#ifdef XDDMAIN
xdd_globals_t   *xgp;   /* pointer to the xdd globals */
#else
extern  xdd_globals_t   *xgp;
#endif


/* The format of the entries in the xdd function table */
typedef int (*func_ptr)(int32_t argc, char *argv[]);

#define XDD_EXT_HELP_LINES 5
struct xdd_func {
	char    *func_name;     /* name of the function */
	char    *func_alt;      /* Alternate name of the function */
    int     (*func_ptr)(int32_t argc, char *argv[]);      /* pointer to the function */
    int     argc;           /* number of arguments */
    char    *help;          /* help string */
    char    *ext_help[XDD_EXT_HELP_LINES];   /* Extented help strings */
}; 
typedef struct xdd_func xdd_func_t;

// Prototypes for parse.c functions
int xddfunc_align(int32_t argc, char *argv[]);
int xddfunc_blocksize(int32_t argc, char *argv[]);
int xddfunc_combinedout(int32_t argc, char *argv[]);
int xddfunc_createnewfiles(int32_t argc, char *argv[]);
int xddfunc_csvout(int32_t argc, char *argv[]);
int xddfunc_datapattern(int32_t argc, char *argv[]);
int xddfunc_delay(int32_t argc, char *argv[]);
int xddfunc_deletefile(int32_t argc, char *argv[]);
int xddfunc_deskew(int32_t argc, char *argv[]);
int xddfunc_devicefile(int32_t argc, char *argv[]);
int xddfunc_dio(int32_t argc, char *argv[]);
int xddfunc_errout(int32_t argc, char *argv[]);
int xddfunc_fullhelp(int32_t argc, char *argv[]);
int xddfunc_heartbeat(int32_t argc, char *argv[]);
int xddfunc_id(int32_t argc, char *argv[]);
int xddfunc_invalid_option(int32_t argc, char *argv[]);
int xddfunc_kbytes(int32_t argc, char *argv[]);
int xddfunc_lockstep(int32_t argc, char *argv[]);
int xddfunc_maxall(int32_t argc, char *argv[]);
int xddfunc_maxerrors(int32_t argc, char *argv[]);
int xddfunc_maxpri(int32_t argc, char *argv[]);
int xddfunc_mbytes(int32_t argc, char *argv[]);
int xddfunc_minall(int32_t argc, char *argv[]);
int xddfunc_nobarrier(int32_t argc, char *argv[]);
int xddfunc_nomemlock(int32_t argc, char *argv[]);
int xddfunc_noproclock(int32_t argc, char *argv[]);
int xddfunc_numreqs(int32_t argc, char *argv[]);
int xddfunc_operation(int32_t argc, char *argv[]);
int xddfunc_output(int32_t argc, char *argv[]);
int xddfunc_passes(int32_t argc, char *argv[]);
int xddfunc_passoffset(int32_t argc, char *argv[]);
int xddfunc_plock(int32_t argc, char *argv[]);
int xddfunc_preallocate(int32_t argc, char *argv[]);
int xddfunc_processor(int32_t argc, char *argv[]);
int xddfunc_port(int32_t argc, char *argv[]);
int xddfunc_qthreadinfo(int32_t argc, char *argv[]);
int xddfunc_queuedepth(int32_t argc, char *argv[]);
int xddfunc_randomize(int32_t argc, char *argv[]);
int xddfunc_readafterwrite(int32_t argc, char *argv[]);
int xddfunc_reallyverbose(int32_t argc, char *argv[]);
int xddfunc_recreatefiles(int32_t argc, char *argv[]);
int xddfunc_reopen(int32_t argc, char *argv[]);
int xddfunc_report_threshold(int32_t argc, char *argv[]);
int xddfunc_reqsize(int32_t argc, char *argv[]);
int xddfunc_roundrobin(int32_t argc, char *argv[]);
int xddfunc_runtime(int32_t argc, char *argv[]);
int xddfunc_rwratio(int32_t argc, char *argv[]);
int xddfunc_seek(int32_t argc, char *argv[]);
int xddfunc_setup(int32_t argc, char *argv[]);
int xddfunc_sgio(int32_t argc, char *argv[]);
int xddfunc_sharedmemory(int32_t argc, char *argv[]);
int xddfunc_singleproc(int32_t argc, char *argv[]);
int xddfunc_startdelay(int32_t argc, char *argv[]);
int xddfunc_startoffset(int32_t argc, char *argv[]);
int xddfunc_starttime(int32_t argc, char *argv[]);
int xddfunc_starttrigger(int32_t argc, char *argv[]);
int xddfunc_stoptrigger(int32_t argc, char *argv[]);
int xddfunc_syncio(int32_t argc, char *argv[]);
int xddfunc_syncwrite(int32_t argc, char *argv[]);
int xddfunc_target(int32_t argc, char *argv[]);
int xddfunc_targetdir(int32_t argc, char *argv[]);
int xddfunc_targetoffset(int32_t argc, char *argv[]);
int xddfunc_targets(int32_t argc, char *argv[]);
int xddfunc_targetstartdelay(int32_t argc, char *argv[]);
int xddfunc_throttle(int32_t argc, char *argv[]);
int xddfunc_timerinfo(int32_t argc, char *argv[]);
int xddfunc_timelimit(int32_t argc, char *argv[]);
int xddfunc_timeserver(int32_t argc, char *argv[]);
int xddfunc_timestamp(int32_t argc, char *argv[]);
int xddfunc_verbose(int32_t argc, char *argv[]);
int xddfunc_verify(int32_t argc, char *argv[]);
int xddfunc_version(int32_t argc, char *argv[]);
int xdd_parse_target_number(int32_t argc, char *argv[], int *target_number);
