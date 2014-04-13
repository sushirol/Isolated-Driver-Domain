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
 * This file contains the subroutines that perform various initialization 
 * functions when xdd is started.
 */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* xdd_init_globals() - Initialize a xdd global variables  
 */
void
xdd_init_globals(void) {

    xgp->global_options = 0;         /* I/O Options valid for all targets */
    xgp->canceled = 0;       /* Normally set to 0. Set to 1 by xdd_sigint when an interrupt occurs */
    xgp->id_firsttime = 0;           /* ID first time through flag */
    xgp->run_ring = 0;       /* The alarm that goes off when the total run time has been exceeded */
    xgp->deskew_ring = 0;    /* The alarm that goes off when the the first thread finishes */
    xgp->abort_io = 0;       /* abort the run due to some catastrophic failure */
    xgp->progname = "xdd";     /* Program name from argv[0] */
    xgp->passes = 0;                 /* number of passes to perform */
    xgp->passdelay = 0;              /* number of seconds to delay between passes */
    xgp->max_errors = 1;             /* max number of errors to tollerate */
    xgp->output_filename = NULL;       /* name of the output file */
    xgp->errout_filename = NULL;       /* name fo the error output file */
    xgp->csvoutput_filename = NULL;    /* name of the csv output file */
    xgp->combined_output_filename = NULL; /* name of the combined output file */
    xgp->tsbinary_filename = NULL;     /* timestamp filename prefix */
    xgp->tsoutput_filename = NULL;     /* timestamp report output filename prefix */
    xgp->output = stdout;                /* Output file pointer*/ 
    xgp->errout = stdout;                /* Error Output file pointer*/ 
    xgp->csvoutput = stdout;             /* Comma Separated Values output file */
    xgp->combined_output = stdout;       /* Combined output file */
    xgp->heartbeat = 0;              /* seconds between heartbeats */
    xgp->syncio = 0;                 /* the number of I/Os to perform btw syncs */
    xgp->target_offset = 0;          /* offset value */
    xgp->number_of_targets = 0;      /* number of targets to operate on */
    xgp->number_of_iothreads = 0;    /* number of threads spawned for all targets */
    xgp->id = "NO ID";                    /* ID string pointer */
    xgp->runtime = 0;                /* Length of time to run all targets, all passes */
    xgp->estimated_end_time = 0;     /* The time at which this run (all passes) should end */
    xgp->number_of_processors = 0;   /* Number of processors */ 
    xgp->random_initialized = 0;     /* Random number generator has not been initialized  */ 
/* information needed to access the Global Time Server */
    xgp->gts_addr = 0;               /* Clock Server IP address */
    xgp->gts_port = 0;               /* Clock Server Port number */
    xgp->gts_time = 0;               /* global time on which to sync */
    xgp->gts_seconds_before_starting = 0; /* number of seconds before starting I/O */
    xgp->gts_bounce = 0;             /* number of times to bounce the time off the global time server */
    xgp->gts_delta = 0;              /* Time difference returned by the clock initializer */
    xgp->gts_hostname = " ";          /* name of the time server */
    xgp->ActualLocalStartTime = 0;   /* The time to start operations */

/* teporary until DESKEW results are fixed */
    xgp->deskew_total_rates = 0;
    xgp->deskew_total_time = 0;
    xgp->deskew_total_bytes = 0;

} /* end of xdd_init_globals() */

/*----------------------------------------------------------------------------*/
/* xdd_init_ptds() - Initialize a Per-Target-Data-Structure 
 */
void
xdd_init_ptds(ptds_t *p, int32_t n) {
		p->mynum = n;
		p->myqnum = 0; 
		p->mypid = getpid();
		p->mythreadid = 0; /* This is set later by the actual thread */
		p->thread_complete = 0;
		p->nextp = 0;
		p->pm1 = 0;
		p->rwbuf = 0;
		p->rwbuf_shmid = -1;
		p->rwbuf_save = 0;
		p->targetdir = DEFAULT_TARGETDIR;
		p->target = DEFAULT_TARGET;
		sprintf(p->targetext,"%08d",1);
		p->reqsize = DEFAULT_REQSIZE;
		p->throttle = 0.0;
		p->throttle_variance = 0.0;
		p->throttle_type = RX_THROTTLE_BW;
		p->ts_options = 0;
		p->target_options = 0;
		p->time_limit = 0;
		p->numreqs = 0;
		p->report_threshold = 0;
		p->kbytes = 0;
		p->mbytes = 0;
		p->start_offset = 0;
		p->pass_offset = 0;
		p->preallocate = 0;
		p->queue_depth = DEFAULT_QUEUEDEPTH;
		p->data_pattern_filename = 0;
		p->data_pattern = "\0";
		p->data_pattern_length = 1;
		p->block_size = DEFAULT_BLOCKSIZE;
		p->align = getpagesize();
        p->my_current_elapsed_time = 0;
        p->my_current_end_time = 0;
        p->my_current_start_time = 0;
        p->my_end_time = 0;
        p->my_start_time = 0;
		p->processor = -1;
		p->start_delay = 0;
		p->start_trigger_time = 0; /* Time to trigger another target to start */
		p->stop_trigger_time = 0; /* Time to trigger another target to stop */
		p->start_trigger_op = 0; /* Operation number to trigger another target to start */
		p->stop_trigger_op = 0; /* Operation number  to trigger another target to stop */
		p->start_trigger_percent = 0; /* Percentage of ops before triggering another target to start */
		p->stop_trigger_percent = 0; /* Percentage of ops before triggering another target to stop */
		p->start_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to start */
		p->stop_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to stop */
		p->start_trigger_target = -1; /* The number of the target to send the start trigger to */
		p->stop_trigger_target = -1; /* The number of the target to send the stop trigger to */
		p->run_status = 1;   /* This is the status of this thread 0=not started, 1=running */
		p->trigger_types = 0;
		p->ls_master = -1; /* The default master number  */
		p->ls_slave  = -1; /* The default slave number */
		p->ls_interval_type  = 0; /* The default interval type */
		p->ls_interval_value  = 0; /* The default interval value  */
		p->ls_interval_units  = "not defined"; /* The default interval units  */
		p->ls_task_type  = 0; /* The default task type */
		p->ls_task_value  = 0; /* The default task value  */
		p->ls_task_units  = "not defined"; /* The default task units  */
		p->ls_task_counter = 0; /* the default task counter */
		/* Init the seeklist header fields */
		p->seekhdr.seek_options = 0;
		p->seekhdr.seek_range = DEFAULT_RANGE;
		p->seekhdr.seek_seed = DEFAULT_SEED;
		p->seekhdr.seek_interleave = DEFAULT_INTERLEAVE;
		p->seekhdr.seek_iosize = DEFAULT_REQSIZE*DEFAULT_BLOCKSIZE;
		p->seekhdr.seek_num_rw_ops = DEFAULT_NUMREQS;
		p->seekhdr.seek_total_ops = DEFAULT_NUMREQS;
		p->seekhdr.seek_NumSeekHistBuckets = DEFAULT_NUM_SEEK_HIST_BUCKETS;/* Number of buckets for seek histogram */
		p->seekhdr.seek_NumDistHistBuckets = DEFAULT_NUM_DIST_HIST_BUCKETS;/* Number of buckets for distance histogram */
		p->seekhdr.seek_savefile = NULL; /* file to save seek locations into */
		p->seekhdr.seek_loadfile = NULL; /* file from which to load seek locations from */
		p->seekhdr.seek_pattern = "sequential";
		/* Init the read-after-write fields */
		p->raw_sd = 0; /* raw socket descriptor */
		p->raw_hostname = NULL;  /* Reader hostname */
		p->raw_lag = DEFAULT_RAW_LAG; 
		p->raw_port = DEFAULT_RAW_PORT;
		p->raw_trigger = RX_RAW_MP; /* default to a message passing */
} /* end of xdd_init_ptds() */
/*----------------------------------------------------------------------------*/
/* xdd_system_info() - Display information about the system this program
 * is being run on. This includes hardware, software, and environmental info.
 */
void
xdd_system_info(FILE *out) {
#if (SOLARIS || IRIX || LINUX || AIX)
	int32_t page_size;
	int32_t physical_pages;
	int32_t memory_size;
	struct utsname name;
#if ( IRIX )
	inventory_t *inventp;
	int64_t mem_size;
#endif // IRIX inventory
	uname(&name);
	fprintf(out, "Computer Name, %s, User Name, %s\n",name.nodename, getlogin());
	fprintf(out, "OS release and version, %s %s %s\n",name.sysname, name.release, name.version);
	fprintf(out, "Machine hardware type, %s\n",name.machine);
#if (SOLARIS)
	xgp->number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
	physical_pages = sysconf(_SC_PHYS_PAGES);
	page_size = sysconf(_SC_PAGE_SIZE);
#elif (AIX)
	xgp->number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
	physical_pages = sysconf(_SC_PHYS_PAGES);
	page_size = sysconf(_SC_PAGE_SIZE);
#elif (LINUX)
	xgp->number_of_processors = xdd_linux_cpu_count();
	physical_pages = sysconf(_SC_PHYS_PAGES);
	page_size = sysconf(_SC_PAGE_SIZE);
#elif (IRIX )
	xgp->number_of_processors = sysconf(_SC_NPROC_ONLN);
	page_size = sysconf(_SC_PAGE_SIZE);
	physical_pages = 0;
	setinvent();
	inventp = getinvent();
	while (inventp) {
		if ((inventp->inv_class == INV_MEMORY) && 
			(inventp->inv_type == INV_MAIN_MB)) {
			mem_size = inventp->inv_state;
			mem_size *= (1024 * 1024);
			physical_pages = mem_size / page_size;
			break;
		}
		inventp = getinvent();
	}
#endif
	fprintf(out, "Number of processors on this system, %d\n",xgp->number_of_processors);
	memory_size = (physical_pages * (page_size/1024))/1024;
	fprintf(out, "Page size in bytes, %d\n",page_size);
	fprintf(out, "Number of physical pages, %d\n", physical_pages);
	fprintf(out, "Megabytes of physical memory, %d\n", memory_size);
#elif (WIN32)
	SYSTEM_INFO system_info; /* Structure to receive system information */
	OSVERSIONINFOEXA osversion_info;
	char computer_name[256];
	DWORD szcomputer_name = sizeof(computer_name);
	char user_name[256];
	DWORD szuser_name = sizeof(user_name);
	MEMORYSTATUS memorystatus;
	BOOL i;
	LPVOID lpMsgBuf;
	GetSystemInfo(&system_info);
	osversion_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	i = GetVersionEx((LPOSVERSIONINFOA)&osversion_info);
	if (i == 0) { 
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			fprintf(xgp->errout,"%s: Error getting version\n",xgp->progname);
			fprintf(xgp->errout,"reason:%s",lpMsgBuf);
	}
	GetComputerName(computer_name,&szcomputer_name);
	GetUserName(user_name,&szuser_name);
	GlobalMemoryStatus(&memorystatus);
	fprintf(out, "Computer Name, %s, User Name, %s\n",computer_name, user_name);
	fprintf(out, "Operating System Info: %s %d.%d Build %d %s\n",
		((osversion_info.dwPlatformId == VER_PLATFORM_WIN32_NT) ? "NT":"Windows95"),
		osversion_info.dwMajorVersion, osversion_info.dwMinorVersion,
		osversion_info.dwBuildNumber,
		osversion_info.szCSDVersion);
	fprintf(out, "Page size in bytes, %d\n",system_info.dwPageSize);
	fprintf(out, "Number of processors on this system, %d\n", system_info.dwNumberOfProcessors);
	fprintf(out, "Megabytes of physical memory, %d\n", memorystatus.dwTotalPhys/(1024*1024));
#endif // WIN32

#ifdef WIN32
	fprintf(out,"Seconds before starting, %I64d\n",xgp->gts_seconds_before_starting);
#else
	fprintf(out,"Seconds before starting, %lld\n",xgp->gts_seconds_before_starting);
#endif
} /* end of xdd_system_info() */

/*----------------------------------------------------------------------------*/
/* xdd_options_info() - Display command-line options information about this run.
 */
void
xdd_options_info(FILE *out) {
	char *c; 


	fprintf(xgp->output,"IOIOIOIOIOIOIOIOIOIOI XDD version %s IOIOIOIOIOIOIOIOIOIOIOI\n",XDD_VERSION);
	fprintf(out,"%s\n",XDD_COPYRIGHT);
	xgp->current_time_for_this_run = time(NULL);
	c = ctime(&xgp->current_time_for_this_run);
	fprintf(out,"Starting time for this run, %s\n",c);
	fprintf(out,"ID for this run, '%s'\n", xgp->id);
	fprintf(out,"Maximum Process Priority, %s", (xgp->global_options & RX_MAXPRI)?"enabled\n":"disabled\n");
	fprintf(out, "Passes, %d\n", xgp->passes);
	fprintf(out, "Pass Delay in seconds, %d\n", xgp->passdelay); 
#ifdef WIN32
		fprintf(out, "Maximum Error Threshold, %I64d\n", xgp->max_errors);
		fprintf(out, "Target Offset, %I64d\n",xgp->target_offset);
#else
		fprintf(out, "Maximum Error Threshold, %lld\n", xgp->max_errors);
		fprintf(out, "Target Offset, %lld\n",xgp->target_offset);
#endif
	fprintf(out, "I/O Synchronization, %d\n", xgp->syncio);
	fprintf(out, "Total run-time limit in seconds, %d\n", xgp->runtime);
	fprintf(out, "Output file name, %s\n",xgp->output_filename);
	fprintf(out, "CSV output file name, %s\n",xgp->csvoutput_filename);
	fprintf(out, "Error output file name, %s\n",xgp->errout_filename);
	if (xgp->global_options & RX_COMBINED)
		fprintf(out,"Combined output file name, %s\n",xgp->combined_output_filename);
	fprintf(out,"Pass seek randomization, %s", (xgp->global_options & RX_PASSRANDOMIZE)?"enabled\n":"disabled\n");
	fprintf(out,"File write synchronization, %s", (xgp->global_options & RX_SYNCWRITE)?"enabled\n":"disabled\n");
	fprintf(out,"Pass synchronization barriers, %s", (xgp->global_options & RX_NOBARRIER)?"disabled\n":"enabled\n");
	if (xgp->gts_hostname) {
			fprintf(out,"Timeserver hostname, %s\n", xgp->gts_hostname);
			fprintf(out,"Timeserver port number, %d\n", xgp->gts_port);
#ifdef WIN32
			fprintf(out,"Global start time, %I64d\n", xgp->gts_time/TRILLION);
#else
			fprintf(out,"Global start time, %lld\n", xgp->gts_time/TRILLION);
#endif
	}
	fprintf(out,"Number of Targets, %d\n",xgp->number_of_targets);
	fprintf(out,"Number of I/O Threads, %d\n",xgp->number_of_iothreads);
	fprintf(out, "\n");
	fflush(out);
} /* end of xdd_options_info() */

/*----------------------------------------------------------------------------*/
/* xdd_target_info() - Display command-line options information about this run.
 */
void
xdd_target_info(FILE *out, ptds_t *p) {
	int i;
	ptds_t *mp, *sp; /* Master and Slave ptds pointers */

    // Only display information for qthreads if requested
    if (!(xgp->global_options & RX_QTHREAD_INFO) && (p->myqnum > 0))
        return;

	fprintf(out,"\tTarget[%d] Q[%d], %s\n",p->mynum, p->myqnum, p->target);
	fprintf(out,"\t\tTarget directory, %s\n",(p->targetdir=="")?"\"./\"":p->targetdir);
	fprintf(out,"\t\tProcess ID, %d\n",p->mypid);
	fprintf(out,"\t\tThread ID, %d\n",p->mythreadid);
    if (p->processor == -1) 
		    fprintf(out,"\t\tProcessor, all/any\n");
	else fprintf(out,"\t\tProcessor, %d\n",p->processor);
	fprintf(out,"\t\tRead/write ratio, %5.2f, %5.2f\n",p->rwratio*100.0,(1.0-p->rwratio)*100.0);
	fprintf(out,"\t\tThrottle in %s, %6.2f\n",(
		p->throttle_type & RX_THROTTLE_OPS)?"ops/sec":"MB/sec",
		p->throttle);
	fprintf(out,"\t\tPer-pass time limit in seconds, %d\n",p->time_limit);
	fprintf(out, "\t\tBlocksize in bytes, %d\n", p->block_size);
	fprintf(out,"\t\tRequest size, %d, blocks, %d, bytes\n",p->reqsize,p->reqsize*p->block_size);
#ifdef WIN32
		if (p->numreqs > 0)
			fprintf(out, "\t\tNumber of Requests, %I64d\n", p->numreqs);
		fprintf(out, "\t\tStart offset, %I64d\n",p->start_offset);
#else
		if (p->numreqs > 0) 
			fprintf(out, "\t\tNumber of Requests, %lld\n", p->numreqs);
		fprintf(out, "\t\tStart offset, %lld\n",p->start_offset);
#endif
		if (p->kbytes > 0)
			fprintf(out, "\t\tNumber of KiloBytes, %d\n", p->kbytes);
		else if (p->mbytes > 0)
			fprintf(out, "\t\tNumber of MegaBytes, %d\n", p->mbytes);
#ifdef WIN32
		fprintf(out, "\t\tPass Offset in blocks, %I64d\n", p->pass_offset);
#else
		fprintf(out, "\t\tPass Offset in blocks, %lld\n", p->pass_offset);
#endif
		fprintf(out,"\t\tI/O memory buffer is %s\n", 
			(p->target_options & RX_SHARED_MEMORY)?"a shared memory segment":"a normal memory buffer");
		fprintf(out,"\t\tI/O memory buffer alignment in bytes, %d\n", p->align);
		fprintf(out,"\t\tData pattern in buffer,");
		if (p->target_options & RX_RANDOM_PATTERN) fprintf(out," random\n");
		else if (p->target_options & RX_SEQUENCED_PATTERN) fprintf(out," sequenced\n");
		else if (p->target_options & RX_ASCII_PATTERN) fprintf(out," ASCII: '%s' <%d bytes> %s\n",
			p->data_pattern,p->data_pattern_length, (p->target_options & RX_REPLICATE_PATTERN)?"Replicated":"Not Replicated");
		else if (p->target_options & RX_HEX_PATTERN) {
			fprintf(out," HEX: 0x");
			for (i=0; i<p->data_pattern_length; i++) 
				fprintf(out,"%02x",p->data_pattern[i]);
			fprintf(out, " <%d bytes>, %s\n",
				p->data_pattern_length, (p->target_options & RX_REPLICATE_PATTERN)?"Replicated":"Not Replicated");
		}

		else if (p->target_options & RX_FILE_PATTERN) fprintf(out," From file: %s\n",p->data_pattern_filename);
		else fprintf(out," '0x%02x'\n",*p->data_pattern);
		fprintf(out,"\t\tData buffer verification is");
		if ((p->target_options & (RX_VERIFY_LOCATION | RX_VERIFY_CONTENTS)))
			fprintf(out," enabled for %s verification.\n", (p->target_options & RX_VERIFY_LOCATION)?"Location":"Content");
		else fprintf(out," disabled.\n");
		fprintf(out,"\t\tDirect I/O, %s", (p->target_options & RX_DIO)?"enabled\n":"disabled\n");
		fprintf(out, "\t\tSeek pattern, %s\n", p->seekhdr.seek_pattern);
		if (p->seekhdr.seek_range > 0)
#ifdef WIN32
			fprintf(out, "\t\tSeek range, %I64d\n",p->seekhdr.seek_range);
#else
			fprintf(out, "\t\tSeek range, %lld\n",p->seekhdr.seek_range);
#endif
		fprintf(out, "\t\tPreallocation, %d\n",p->preallocate);
		fprintf(out, "\t\tQueue Depth, %d\n",p->queue_depth);
		/* Timestamp options */
		if (p->ts_options & TS_ON) {
			fprintf(out, "\t\tTimestamping, enabled for %s %s\n",(p->ts_options & TS_DETAILED)?"DETAILED":"", (p->ts_options & TS_SUMMARY)?"SUMMARY":"");
			fprintf(out, "\t\tTimestamp ASCII output file name, %s.target.%04d.qthread.%04d.csv\n",xgp->tsoutput_filename,p->mynum,p->myqnum);
			if (p->ts_options & TS_DUMP) 
				fprintf(out, "\t\tTimestamp binary output file name, %s.target.%04d.qthread.%04d.bin\n",xgp->tsbinary_filename,p->mynum,p->myqnum);
		} else fprintf(out, "\t\tTimestamping, disabled\n");
			fflush(out);
		fprintf(out,"\t\tDelete file, %s", (p->target_options & RX_DELETEFILE)?"enabled\n":"disabled\n");
		if (p->myqnum == 0) {
			if (p->ls_master >= 0) {
				mp = &xgp->ptds[p->ls_master];
				fprintf(out,"\t\tMaster Target, %d\n", p->ls_master);
#ifdef WIN32
				fprintf(out,"\t\tMaster Interval value and type, %I64d,%s\n", mp->ls_interval_value, mp->ls_interval_units);
#else
				fprintf(out,"\t\tMaster Interval value and type, %lld,%s\n", mp->ls_interval_value, mp->ls_interval_units);
#endif
			}
			if (p->ls_slave >= 0) {
				sp = &xgp->ptds[p->ls_slave];
				fprintf(out,"\t\tSlave Target, %d\n", p->ls_slave);
#ifdef WIN32
				fprintf(out,"\t\tSlave Task value and type, %I64d,%s\n", sp->ls_task_value,sp->ls_task_units);
#else
				fprintf(out,"\t\tSlave Task value and type, %lld,%s\n", sp->ls_task_value,sp->ls_task_units);
#endif
				fprintf(out,"\t\tSlave initial condition, %s\n",(sp->ls_ms_state & LS_SLAVE_RUN_IMMEDIATELY)?"Run":"Wait");
				fprintf(out,"\t\tSlave termination, %s\n",(sp->ls_ms_state & LS_SLAVE_COMPLETE)?"Complete":"Abort");
			}
        }
	fprintf(out, "\n");
	fflush(out);
} /* end of xdd_target_info() */

/*----------------------------------------------------------------------------*/
/* xdd_config_info() - Display configuration information about this run.
 */
void
xdd_config_info(void) {
	xdd_options_info(xgp->output);
	xdd_system_info(xgp->output);
	if (xgp->global_options & RX_CSV) {
		xdd_options_info(xgp->csvoutput);
		xdd_system_info(xgp->csvoutput);
	}
	fflush(xgp->output);
} /* end of xdd_config_info() */
/*----------------------------------------------------------------------------*/
/* xdd_sigint() - Routine that gets called when an Interrupt occurs. This will
 * call the appropriate routines to remove all the barriers and semaphores so
 * that we shut down gracefully.
 */
void
xdd_sigint(int n) {
	fprintf(xgp->errout,"Program canceled - destroying all barriers...");
	fflush(xgp->errout);
	xgp->canceled = 1;
	xdd_destroy_all_barriers();
	fprintf(xgp->errout,"done. Exiting\n");
	fflush(xgp->errout);
} /* end of xdd_sigint() */

/*----------------------------------------------------------------------------*/
/* xdd_init_signals() - Initialize all the signal handlers
 */
void
xdd_init_signals(void) {
	signal(SIGINT, xdd_sigint);
} /* end of xdd_init_signals() */
/*----------------------------------------------------------------------------*/
/* xdd_init_global_clock_network() - Initialize the network so that we can
 *   talk to the global clock timeserver.
 *   
 */
in_addr_t
xdd_init_global_clock_network(char *hostname) {
	struct hostent *hostent; /* used to init the time server info */
	in_addr_t addr;  /* Address of hostname from hostent */
#ifdef WIN32
	WSADATA wsaData; /* Data structure used by WSAStartup */
	int wsastatus; /* status returned by WSAStartup */
	char *reason;
	wsastatus = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsastatus != 0) { /* Error in starting the network */
		switch (wsastatus) {
		case WSASYSNOTREADY:
			reason = "Network is down";
			break;
		case WSAVERNOTSUPPORTED:
			reason = "Request version of sockets <2.2> is not supported";
			break;
		case WSAEINPROGRESS:
			reason = "Another Windows Sockets operation is in progress";
			break;
		case WSAEPROCLIM:
			reason = "The limit of the number of sockets tasks has been exceeded";
			break;
		case WSAEFAULT:
			reason = "Program error: pointer to wsaData is not valid";
			break;
		default:
			reason = "Unknown error code";
			break;
		};
		fprintf(xgp->errout,"%s: Error initializing network connection\nReason: %s\n",
			xgp->progname, reason);
		fflush(xgp->errout);
		WSACleanup();
		return(-1);
	} 
	/* Check the version number */
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		/* Couldn't find WinSock DLL version 2.2 or better */
		fprintf(xgp->errout,"%s: Error initializing network connection\nReason: Could not find version 2.2\n",
			xgp->progname);
		fflush(xgp->errout);
		WSACleanup();
		return(-1);
	}
#endif
	/* Network is initialized and running */
	hostent = gethostbyname(hostname);
	if (!hostent) {
		fprintf(xgp->errout,"%s: Error: Unable to identify host %s\n",xgp->progname,hostname);
		fflush(xgp->errout);
#ifdef WIN32
		WSACleanup();
#endif
		return(-1);
	}
	/* Got it - unscramble the address bytes and return to caller */
	addr = ntohl(((struct in_addr *)hostent->h_addr)->s_addr);
	return(addr);
} /* end of xdd_init_global_clock_network() */
/*----------------------------------------------------------------------------*/
/* xdd_init_global_clock() - Initialize the global clock if requested
 */
void
xdd_init_global_clock(pclk_t *pclkp) {
	pclk_t  now;  /* the current time returned by pclk() */


	/* Global clock stuff here */
	if (xgp->gts_hostname) {
		xgp->gts_addr = xdd_init_global_clock_network(xgp->gts_hostname);
		if (xgp->gts_addr == -1) { /* Problem with the network */
			fprintf(xgp->errout,"%s: Error initializing global clock - network malfunction\n",xgp->progname);
			fflush(xgp->errout);
                        *pclkp = 0;
			return;
		}
		clk_initialize(xgp->gts_addr, xgp->gts_port, xgp->gts_bounce, &xgp->gts_delta);
		pclk_now(&now);
		xgp->ActualLocalStartTime = xgp->gts_time - xgp->gts_delta; 
		xgp->gts_seconds_before_starting = ((xgp->ActualLocalStartTime - now) / TRILLION); 
#ifdef WIN32
fprintf(xgp->errout,"Global Time now is %I64d. Starting in %I64d seconds at Global Time %I64d\n",
#else
fprintf(xgp->errout,"Global Time now is %lld. Starting in %lld seconds at Global Time %lld\n",
#endif
			(now+xgp->gts_delta)/TRILLION, 
			xgp->gts_seconds_before_starting, 
			xgp->gts_time/TRILLION); 
		fflush(xgp->errout);
		*pclkp = xgp->ActualLocalStartTime;
		return;
	}
	pclk_now(pclkp);
	return;
} /* end of xdd_init_global_clock() */

/*----------------------------------------------------------------------------*/
/* xdd_pattern_buffer() - init the I/O buffer with the appropriate pattern
 * This routine will put the requested pattern in the rw buffer.
 */
void
xdd_pattern_buffer(ptds_t *p) {
	int32_t i;
	int32_t pattern_length; // Length of the pattern
	int32_t remaining_length; // Length of the space in the pattern buffer
	unsigned char    *ucp;          // Pointer to an unsigned char type, duhhhh
	uint32_t *lp;			// pointer to a pattern


	if (p->target_options & RX_RANDOM_PATTERN) { // A nice random pattern
			lp = (uint32_t *)p->rwbuf;
			xgp->random_initialized = 0;
            /* Set each four-byte field in the I/O buffer to a random integer */
			for(i = 0; i < (int32_t)(p->iosize / sizeof(int32_t)); i++ ) {
				*lp=xdd_random_int();
				lp++;
			}
	} else if ((p->target_options & RX_ASCII_PATTERN) ||
	           (p->target_options & RX_HEX_PATTERN)) { // put the pattern that is in the pattern buffer into the io buffer
			// Clear out the buffer before putting in the string so there are no strange characters in it.
			memset(p->rwbuf,'\0',p->iosize);
			if (p->target_options & RX_REPLICATE_PATTERN) { // Replicate the pattern throughout the buffer
				ucp = (unsigned char *)p->rwbuf;
				remaining_length = p->iosize;
				while (remaining_length) { 
					if (p->data_pattern_length < remaining_length) 
						pattern_length = p->data_pattern_length;
					else pattern_length = remaining_length;

					memcpy(ucp,p->data_pattern,pattern_length);
					remaining_length -= pattern_length;
					ucp += pattern_length;
				}
			} else { // Just put the pattern at the beginning of the buffer once 
				if (p->data_pattern_length < p->iosize) 
					 pattern_length = p->data_pattern_length;
				else pattern_length = p->iosize;
				memcpy(p->rwbuf,p->data_pattern,pattern_length);
			}
		} else { // Otherwise set the entire buffer to the character in "data_pattern"
			memset(p->rwbuf,*(p->data_pattern),p->iosize);
		}
		
	return;
} /* end of xdd_process_directive() */
/*----------------------------------------------------------------------------*/
/* xdd_init_io_buffers() - set up the I/O buffers
 * This routine will allocate the memory used as the I/O buffer for a target.
 * For some operating systems, you can use a shared memory segment instead of 
 * a normal malloc/valloc memory chunk. This is done using the "-sharedmemory"
 * command line option.
 */
unsigned char *
xdd_init_io_buffers(ptds_t *p) {
	unsigned char *rwbuf; /* the read/write buffer for this op */
#ifdef WIN32
	LPVOID lpMsgBuf; /* Used for the error messages */
#endif


	/* Check to see if we want to use a shared memory segment and allocate it using shmget() and shmat().
	 * NOTE: This is not supported by all operating systems. 
	 */
	if (p->target_options & RX_SHARED_MEMORY) {
#if (AIX || LINUX || SOLARIS || OSX)
		/* In AIX we need to get memory in a shared memory segment to avoid
	     * the system continually trying to pin each page on every I/O operation */
#if (AIX)
		p->rwbuf_shmid = shmget(IPC_PRIVATE, p->iosize, IPC_CREAT | SHM_LGPAGE |SHM_PIN );
#else
		p->rwbuf_shmid = shmget(IPC_PRIVATE, p->iosize, IPC_CREAT );
#endif
		if (p->rwbuf_shmid < 0) {
			fprintf(xgp->errout,"%s: Cannot create shared memory segment\n", xgp->progname);
			perror("Reason");
			rwbuf = 0;
			p->rwbuf_shmid = -1;
		} else {
			rwbuf = (unsigned char *)shmat(p->rwbuf_shmid,0,0);
			if (rwbuf < (unsigned char *)0) {
				fprintf(xgp->errout,"%s: Cannot attach to shared memory segment\n",xgp->progname);
				perror("Reason");
				rwbuf = 0;
				p->rwbuf_shmid = -1;
			}
		}
		if (xgp->global_options & RX_REALLYVERBOSE)
				fprintf(xgp->output,"Shared Memory ID allocated and attached, shmid=%d\n",p->rwbuf_shmid);
#elif (IRIX || HPUX || WIN32 || ALTIX)
		fprintf(xgp->errout,"%s: Shared Memory not supported on this OS - using valloc\n",
			xgp->progname);
		p->target_options &= ~RX_SHARED_MEMORY;
#if (IRIX || SOLARIS || HPUX || LINUX || AIX || ALTIX || OSX)
		rwbuf = valloc(p->iosize);
#else
		rwbuf = malloc(p->iosize);
#endif
#endif 
	} else { /* Allocate memory the normal way */
#if (IRIX || SOLARIS || HPUX || LINUX || AIX || ALTIX || OSX)
		rwbuf = valloc(p->iosize);
#else
		rwbuf = malloc(p->iosize);
#endif
	}
	/* Check to see if we really allocated some memory */
	if (rwbuf == NULL) {
		fprintf(xgp->errout,"%s: cannot allocate %d bytes of memory for I/O buffer\n",
			xgp->progname,p->iosize);
		fflush(xgp->errout);
#ifdef WIN32 
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(xgp->errout,"Reason:%s",lpMsgBuf);
		fflush(xgp->errout);
#else 
		perror("Reason");
#endif
		return(NULL);
	}
	/* Memory allocation must have succeeded */

	/* Lock all rwbuf pages in memory */
	xdd_lock_memory(rwbuf, p->iosize, "RW BUFFER");

	return(rwbuf);
} /* end of xdd_init_io_buffers() */

/*----------------------------------------------------------------------------*/
int32_t
xdd_process_directive(char *lp) {
	fprintf(xgp->errout,"directive %s being processed\n",lp);
	return(0);
} /* end of xdd_process_directive() */
/*----------------------------------------------------------------------------*/
char *
xdd_getnexttoken(char *tp) {
	char *cp;
	cp = tp;
	while ((*cp != TAB) && (*cp != SPACE)) cp++;
	while ((*cp == TAB) || (*cp == SPACE)) cp++;
	return(cp);
} /* end of xdd_getnexttoken() */
/*----------------------------------------------------------------------------*/
void
xdd_lock_memory(unsigned char *bp, uint32_t bsize, char *sp) {
	int32_t status; /* status of a system call */

	// If the nomemlock option is set then do not lock memory 
	if (xgp->global_options & RX_NOMEMLOCK)
		return;

#if (AIX)
	int32_t liret;
	off_t newlim;
	/* Get the current memory stack limit - required before locking the the memory */
	liret = -1;
	newlim = -1;
	liret = ulimit(GET_STACKLIM,0);
	if (liret == -1) {
		fprintf(xgp->errout,"(PID %d) %s: AIX Could not get memory stack limit\n",
				getpid(),xgp->progname);
		perror("Reason");
	}
	/* Get 8 pages more than the stack limit */
	newlim = liret - (PAGESIZE*8);
	return;
#else
#if  (LINUX || SOLARIS || HPUX || OSX || AIX)
	if (getuid() != 0) {
		fprintf(xgp->errout,"(PID %d) %s: You must run as superuser to lock memory for %s\n",
			getpid(),xgp->progname, sp);
		return;
	}
#endif
	status = mlock((char *)bp, bsize);
	if (status < 0) {
			fprintf(xgp->errout,"(PID %d) %s: Could not lock %d bytes of memory for %s\n",
				getpid(),xgp->progname,bsize,sp);
		perror("Reason");
	}
#endif
} /* end of xdd_lock_memory() */
/*----------------------------------------------------------------------------*/
/* xdd_processor() - assign this xdd thread to a specific processor 
 * This works on most operating systems except LINUX at the moment. 
 * The ability to assign a *process* to a specific processor is new to Linux as
 * of the 2.6.8 Kernel. However, the ability to assign a *thread* to a specific
 * processor still does not exist in Linux as of 2.6.8. Therefore, all xdd threads
 * will run on all processors using a "faith-based" approach - in other words you
 * need to place your faith in the Linux kernel thread scheduler that the xdd
 * threads get scheduled on the appropriate processors and evenly spread out
 * accross all processors as efficiently as possible. Scarey huh? If/when the
 * Linux developers provide the ability for individual threads to be assigned
 * to specific processors for Linux that code will be incorporated here.
 */
void
xdd_processor(ptds_t *p) {
#if (SOLARIS)
	processorid_t i;
	int32_t  status;
	int32_t  n,cpus;
	n = sysconf(_SC_NPROCESSORS_ONLN);
	cpus = 0;
	for (i = 0; n > 0; i++) {
		status = p_online(i, P_STATUS);
		if (status == -1 )
			continue;
		/* processor present */
		if (cpus == p->processor) 
			break;
		cpus++;
		n--;
	}
	/* At this point "i" contains the processor number to bind this process to */
	status = processor_bind(P_LWPID, P_MYID, i, NULL);
	if (status < 0) {
		fprintf(xgp->errout,"%s: Processor assignment failed for target %d\n",xgp->progname,p->mynum);
		perror("Reason");
	}
	return;

#elif (LINUXSMP)
	size_t 		cpumask_size; 	/* size of the CPU mask in bytes */
	cpu_set_t 	cpumask; 	/* mask of the CPUs configured on this system */
	int		status; 	/* System call status */
	int32_t 	n; 		/* the number of CPUs configured on this system */
	int32_t 	cpus; 		/* the number of CPUs configured on this system */
	int32_t 	i; 

// This *almost* works. However, threads are all tied to the process that created them hence they all have the same PID.
// Thus when the first sched_setaffinity() call gets made to assign this thread to a processor, all the other processors
// are effectively shut off for this PID and subsequently all the other threads as well. 
// The conclusion as of 3/1/05 is that thread-to-processor affinity binding is still not working in Linux 2.6.8.
#ifdef LINUX_THREAD_PROCESSOR_AFFINITY_IS_WORKING

	cpumask_size = (unsigned int)sizeof(cpumask);
	status = sched_getaffinity(getpid(), cpumask_size, &cpumask);
	if (status != 0) {
		fprintf(xgp->errout,"%s: WARNING: Error getting the CPU mask when trying to schedule processor affinity\n",xgp->progname);
		return;
	}
	n = xdd_linux_cpu_count();
	cpus = 0;

	for (i = 0; n > 0; i++) {
		if (CPU_ISSET(i, &cpumask)) {
			/* processor present */
			if (cpus == p->processor) 
				break;
			cpus++;
			n--;
		}
	}
	/* at this point i contains the proper CPU number to use in the mask setting */
	cpumask_size = (unsigned int)sizeof(cpumask);
	CPU_ZERO(&cpumask);
	CPU_SET(i, &cpumask);
	status = sched_setaffinity(getpid(), cpumask_size, &cpumask);
	if (status != 0) {
		fprintf(xgp->errout,"%s: WARNING: Error setting the CPU mask when trying to schedule processor affinity\n",xgp->progname);
		perror("Reason");
	}
	if (xgp->global_options&RX_REALLYVERBOSE);
		fprintf(xgp->output,"%s: INFORMATION: Assigned processor %d to pid %d threadid %d \n",
			xgp->progname,
			p->processor,
			p->mypid,
			p->mythreadid);
	return;
#else 
		fprintf(xgp->output,"%s: WARNING: Processor/Thread assignment is not possible with this version of Linux.\n",
			xgp->progname);
	return;
#endif // End of the ifdef LINUX_THREAD_PROCESSOR_AFFINITY_IS_WORKING 

#elif (AIX)
	int32_t status;
	if (xgp->global_options & RX_REALLYVERBOSE)
		fprintf(xgp->output, "Binding process/thread %d/%d to processor %d\n",p->mypid, p->mythreadid, p->processor);
	status = bindprocessor( BINDTHREAD, p->mythreadid, p->processor );
	if (status) {
		fprintf(xgp->errout,"%s: Processor assignment failed for target %d to processor %d, thread ID %d, process ID %d\n",
			xgp->progname, p->mynum, p->processor, p->mythreadid, p->mypid);
		perror("Reason");
	}
	return;
#elif (IRIX)
	int32_t  i;
	i = sysmp (MP_MUSTRUN,p->processor);
	if (i < 0) {
		fprintf(xgp->errout,"%s: **WARNING** Error assigning target %d to processor %d\n",
			xgp->progname, p->mynum, p->processor);
		perror("Reason");
	}
	return;
#endif
} /* end of xdd_processor() */
/*----------------------------------------------------------------------------*/
void
xdd_unlock_memory(unsigned char *bp, uint32_t bsize, char *sp) {
	int32_t status; /* status of a system call */

	// If the nomemlock option is set then do not unlock memory because it was never locked
	if (xgp->global_options & RX_NOMEMLOCK)
		return;
#if (AIX)
#ifdef notdef
	status = plock(UNLOCK);
	if (status) {
		fprintf(xgp->errout,"(PID %d) %s: AIX Could not unlock memory for %s\n",
				getpid(),xgp->progname, sp);
		perror("Reason");
	}
#endif
	return;
#else
#if (IRIX || SOLARIS || HPUX || LINUX || ALTIX || OSX)
	if (getuid() != 0) {
		return;
	}
#endif
	status = munlock((char *)bp, bsize);
	if (status < 0) {
			fprintf(xgp->errout,"(PID %d) %s: Could not unlock memory for %s\n",
				getpid(),xgp->progname,sp);
		perror("Reason");
	}
#endif
} /* end of xdd_unlock_memory() */
/*----------------------------------------------------------------------------*/
/* xdd_random_int() - returns a random integer
 */
int
xdd_random_int(void) {
#ifdef  LINUX


	if (xgp->random_initialized == 0) {
		initstate(72058, xgp->random_init_state, 256);
		xgp->random_initialized = 1;
	}
#endif
#ifdef WIN32
	return(rand());
#else
	return(random());
#endif
} /* end of xdd_random_int() */
/*----------------------------------------------------------------------------*/
/* xdd_random_float() - returns a random floating point number in double.
 */
double
xdd_random_float(void) {
#ifdef WIN32
	return((double)(1.0 / RAND_MAX) * rand());
#else
	return((double)(1.0 / RAND_MAX) * random());
#endif
} /* end of xdd_random_float() */
/*----------------------------------------------------------------------------*/
/* xdd_heartbeat() - periodically display running counters
 */
void *
xdd_heartbeat(void *junk) {  
	int32_t i;
	pclk_t e;
	double d,r;
	ptds_t *p;
	for (;;) {
		fprintf(stderr,"\r");
		fprintf(stderr,"Pass %04d Op/AvgRate ",xgp->ptds[0].current_pass);
		for (i = 0; i < xgp->number_of_targets; i++) {
			p = &xgp->ptds[i];
#ifdef WIN32
			fprintf(stderr,"%06I64d/",p->perpass.ops);
#else
			fprintf(stderr,"%06lld/",p->perpass.ops);
#endif
			if (p->perpass.accum_time > BILLION) {
				e = p->perpass.accum_time/BILLION;
				d = e;
				d /= 1000.0;
				r = (p->perpass.bytes/d)/1000000.0;
				fprintf(stderr,"%5.2f * ",r);
			} else fprintf(stderr,"0 * ");
		}
		sleep(xgp->heartbeat);
	}
} /* end of xdd_heartbeat() */
/*----------------------------------------------------------------------------*/
/* xdd_schedule_options() - do the appropriate scheduling operations to 
 *   maximize performance of this program.
 */
void
xdd_schedule_options(void) {
	int32_t status;  /* status of a system call */
	struct sched_param param; /* for the scheduler */

	if (xgp->global_options & RX_NOPROCLOCK) 
                return;
#if !(OSX)
#if (IRIX || SOLARIS || HPUX || AIX || LINUX || ALTIX || OSX)
	if (getuid() != 0)
		fprintf(xgp->errout,"%s: xdd_schedule_options: You must be super user to lock processes\n",xgp->progname);
#endif 
	/* lock ourselves into memory for the duration */
	status = mlockall(MCL_CURRENT | MCL_FUTURE);
	if (status < 0) {
		fprintf(xgp->errout,"%s: cannot lock process into memory\n",xgp->progname);
		perror("Reason");
	}
	if (xgp->global_options & RX_MAXPRI) {
#if (IRIX || SOLARIS || HPUX || AIX || LINUX || ALTIX || OSX)
		if (getuid() != 0) 
			fprintf(xgp->errout,"%s: xdd_schedule_options: You must be super user to max priority\n",xgp->progname);
#endif
		/* reset the priority to max max max */
		param.sched_priority = sched_get_priority_max(SCHED_FIFO);
		status = sched_setscheduler(0,SCHED_FIFO,&param);
		if (status == -1) {
			fprintf(xgp->errout,"%s: cannot reschedule priority\n",xgp->progname);
			perror("Reason");
		}
	}
#endif // if not OSX
} /* end of xdd_schedule_options() */
/*----------------------------------------------------------------------------*/
/* xdd_open_target() - open the target device and do all necessary 
 * sanity checks.  If the open fails or the target cannot be used  
 * with DIRECT I/O then this routine will exit the program.   
 */
#ifdef WIN32
HANDLE
#else
int32_t
#endif 
xdd_open_target(ptds_t *p) {
	char target_name[MAX_TARGET_NAME_LENGTH]; /* target directory + target name */
#ifdef WIN32
	HANDLE hfile;  /* The file handle */
	LPVOID lpMsgBuf; /* Used for the error messages */
	unsigned long flags; /* Open flags */
	bool status;  /* status of a system call */
	BY_HANDLE_FILE_INFORMATION fileinfo;
	DWORD disp;

	/* create the fully qualified target name */
	memset(target_name,0,sizeof(target_name));
	if (strlen(p->targetdir) > 0)
		sprintf(target_name, "%s%s", p->targetdir, p->target);
	else sprintf(target_name, "%s",p->target);	
	
	if (p->target_options & RX_CREATE_NEW_FILES) { // Add the target extension to the name
		strcat(target_name, ".");
		strcat(target_name, p->targetext);
	}

	/* open the target */
	if (p->target_options & RX_DIO) 
		flags = FILE_FLAG_NO_BUFFERING;
	else flags = 0;
	/* Device files and files that are being read MUST exist
	 * in order to be opened. For files that are being written,
	 * they can be created if they do not exist or use the existing one.
	 */
	if ((p->target_options & RX_DEVICEFILE) || (p->rwratio == 1.0))
		disp = OPEN_EXISTING; 
	else if (p->rwratio < 1.0)
		disp = OPEN_ALWAYS;
	else disp = OPEN_EXISTING;

	pclk_now(&p->open_start_time); // Record the starting time of the open

	if (p->rwratio < 1.0)  { /* open for write operations */
		hfile = CreateFile(target_name, 
			GENERIC_WRITE | GENERIC_READ,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else if (p->rwratio == 0.0) { /* write only */
		hfile = CreateFile(target_name, 
			GENERIC_WRITE,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else if ((p->rwratio > 0.0) && p->rwratio < 1.0) { /* read/write mix */
		hfile = CreateFile(target_name, 
			GENERIC_WRITE | GENERIC_READ,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else { /* open for read operations */
		hfile = CreateFile(target_name, 
			GENERIC_READ, 
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	}
	if (hfile == INVALID_HANDLE_VALUE) {
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(xgp->errout,"%s: could not open target for %s: %s\n",
			xgp->progname,(p->rwratio < 1.0)?"write":"read",target_name);
			fprintf(xgp->errout,"reason:%s",lpMsgBuf);
			fflush(xgp->errout);
			return((void *)-1);
	}
	fileinfo.dwFileAttributes=0;
	if (!(p->target_options & RX_DEVICEFILE)) {
		status = GetFileInformationByHandle(hfile,&fileinfo);
		if (status == 0) {
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			fprintf(xgp->errout,"%s: could not get file information for target %s\n",
				xgp->progname,target_name);
				fprintf(xgp->errout,"reason:%s",lpMsgBuf);
				fflush(xgp->errout);
			// return((void *)-1);
		}
	}
	pclk_now(&p->open_end_time); // Record the ending time of the open
	p->filetype = fileinfo.dwFileAttributes;
	return(hfile);
#else /* UNIX-style open */
#if ( IRIX || ALTIX )
	struct dioattr dioinfo; /* Direct IO information */
	struct flock64 flock; /* Structure used by preallocation fcntl */
#endif
#if (IRIX || SOLARIS || HPUX || AIX || ALTIX)
	struct stat64 statbuf; /* buffer for file statistics */
#elif ( LINUX || OSX )
	struct stat statbuf; /* buffer for file statistics */
#endif
	int32_t  i; /* working variable */
	int32_t  status; /* working variable */
	int32_t  fd; /* the file descriptor */
	int32_t  flags; /* file open flags */

	/* create the fully qualified target name */
	memset(target_name,0,sizeof(target_name));
	if (strlen(p->targetdir) > 0)
		sprintf(target_name, "%s%s", p->targetdir, p->target);
	else sprintf(target_name, "%s",p->target);

	if (p->target_options & RX_CREATE_NEW_FILES) { // Add the target extension to the name
		strcat(target_name, ".");
		strcat(target_name, p->targetext);
	}
	/* Set the open flags according to specific OS requirements */
	flags = O_CREAT;
#if (SOLARIS || AIX)
	flags |= O_LARGEFILE;
#endif
#if (AIX || LINUX)
	/* setup for DIRECTIO for AIX & perform sanity checks */
	if (p->target_options & RX_DIO) {
		/* make sure it is a regular file, otherwise fail */
		flags |= O_DIRECT;
	}
#endif

	pclk_now(&p->open_start_time); // Record the starting time of the open

	/* open the target */
	if (p->rwratio == 0.0) {
#if (HPSS)
		if (p->target_options & RX_HPSS)
			 fd = hpss_Open(target_name,flags|O_WRONLY, 0666, 0, 0, 0); /* write only */
		else fd = open(target_name,flags|O_WRONLY, 0666); /* write only */
#else
        if (p->target_options & RX_SGIO) {
#if (LINUX)
             fd = open(target_name,flags|O_RDWR, 0777); /* Must open RDWR for SGIO */
             i = (p->block_size*p->reqsize);
             status = ioctl(fd, SG_SET_RESERVED_SIZE, &i);
             if (status < 0)
                 fprintf(xgp->errout,"%s: SG_SET_RESERVED_SIZE error - request for %d bytes denied",xgp->progname, (p->block_size*p->reqsize));
             status = ioctl(fd, SG_GET_VERSION_NUM, &i);
             if ((status < 0) || (i < 30000)) 
                 fprintf(xgp->errout, "%s: sg driver prior to 3.x.y - specifically %d\n",xgp->progname,i);
#endif // LINUX SGIO open stuff
        } else fd = open(target_name,flags|O_WRONLY, 0666); /* write only */
#endif
	}
	else if (p->rwratio == 1.0) { /* read only */
		flags &= ~O_CREAT;
#if (HPSS)
		if (p->target_options & RX_HPSS)
			 fd = hpss_Open(target_name,flags|O_RDONLY,0444,0,0,0); 
		else fd = open(target_name,flags|O_RDONLY); 
#else
        if (p->target_options & RX_SGIO) {
#if (LINUX)
		    fd = open(target_name,flags|O_RDWR, 0777); /* Must open RDWR for SGIO  */
            i = (p->block_size*p->reqsize);
            status = ioctl(fd, SG_SET_RESERVED_SIZE, &i);
            if (status < 0)
                fprintf(xgp->errout,"%s: SG_SET_RESERVED_SIZE error - request for %d bytes denied",xgp->progname, (p->block_size*p->reqsize));
            status = ioctl(fd, SG_GET_VERSION_NUM, &i);
            if ((status < 0) || (i < 30000)) 
                fprintf(xgp->errout, "%s: sg driver prior to 3.x.y - specifically %d\n",xgp->progname,i);
#endif // LINUX SGIO open stuff
        } else fd = open(target_name,flags|O_RDONLY, 0777); /* Read only */
#endif
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		flags &= ~O_CREAT;
#if (HPSS)
		if (p->target_options & RX_HPSS)
			 fd = hpss_Open(target_name,flags|O_RDWR, 0666, 0, 0, 0);
		else fd = open(target_name,flags|O_RDWR, 0666);
#else
		fd = open(target_name,flags|O_RDWR, 0666);
#endif
	}

	pclk_now(&p->open_end_time); // Record the ending time of the open

	if (fd < 0) {
			fprintf(xgp->errout,"(%d) %s: could not open target: %s\n",
				p->mynum,xgp->progname,target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
	}
	/* Stat the file so we can do some sanity checks */
#if (IRIX || SOLARIS || HPUX || AIX || ALTIX)
#if (HPSS)
	if (p->target_options & RX_HPSS)
		 i = hpss_Fstat64(fd,&statbuf);
	else i = fstat64(fd,&statbuf);
#else
	i = fstat64(fd,&statbuf);
#endif
#else
	i = fstat(fd,&statbuf);
#endif
	if (i < 0) { /* Check file type */
		fprintf(xgp->errout,"(%d) %s: could not stat target: %s\n",
			p->mynum,xgp->progname,target_name);
		fflush(xgp->errout);
		perror("reason");
		return(-1);
	}
	/* If this is a regular file, and we are trying to read it, then
	 * check to see that we are not going to read off the end of the
	 * file. Send out a WARNING if this is a possibility
	 */
		if ((statbuf.st_mode & S_IFMT) == S_IFREG) 
			p->target_options |= RX_REGULARFILE;
		if ( ((statbuf.st_mode & S_IFMT) == S_IFREG) && !(p->rwratio < 1.0)) {
		if (p->bytestoxfer > statbuf.st_size) {
#ifdef WIN32
			fprintf(xgp->errout,"(%d) %s: WARNING! The target file <%I64d bytes> is shorter than the the total requested transfer size <%I64d bytes>\n",
#else
			fprintf(xgp->errout,"(%d) %s: WARNING! The target file <%lld bytes> is shorter than the the total requested transfer size <%lld bytes>\n",
#endif
				p->mynum,xgp->progname,statbuf.st_size, p->bytestoxfer);
			fflush(xgp->errout);
		}
	}
#if (IRIX || SOLARIS || ALTIX)
	/* setup for DIRECTIO & perform sanity checks */
	if (p->target_options & RX_DIO) {
			/* make sure it is a regular file, otherwise fail */
			if ( (statbuf.st_mode & S_IFMT) != S_IFREG) {
				fprintf(xgp->errout,"(%d) %s: target %s must be a regular file when used with the -dio flag\n",
					p->mynum,xgp->progname,target_name);
				fflush(xgp->errout);
				return(-1);
			}
#if ( IRIX || ALTIX ) /* DIO for IRIX or ALTIX */
			/* set the DIRECTIO flag */
			flags = fcntl(fd,F_GETFL);
			i = fcntl(fd,F_SETFL,flags|FDIRECT);
			if (i < 0) {
				fprintf(xgp->errout,"(%d) %s: could not set DIRECTIO flag for: %s\n",
					p->mynum,xgp->progname,target_name);
				fflush(xgp->errout);
				perror("reason");
				if (errno == EINVAL) {
				fprintf(xgp->errout,"(%d) %s: target %s is not on an EFS or xFS filesystem\n",
					p->mynum,xgp->progname,target_name);
				fflush(xgp->errout);
				}
				return(-1);
		}
		i = fcntl(fd, F_DIOINFO, &dioinfo);
		if (i != 0) {
			fprintf(xgp->errout,"(%d) %s: Cannot get DirectIO info for target %s\n",
					p->mynum,xgp->progname,target_name);
			fflush(xgp->errout);
			perror("Reason");
		} else { /* check the DIO size & alignment */
			/* If the io size is less than the minimum then exit */
			if (p->iosize < dioinfo.d_miniosz) {
fprintf(xgp->errout,"(%d) %s: The iosize of %d bytes is smaller than the minimum DIRECTIO iosize <%d bytes> for target %s\n",
					p->mynum,xgp->progname,p->iosize,dioinfo.d_miniosz,target_name);
				fflush(xgp->errout);
				return(-1);
			}
			if (p->iosize > dioinfo.d_maxiosz) {
fprintf(xgp->errout,"(%d) %s: The iosize of %d bytes is greater than the maximum DIRECTIO iosize <%d bytes> for target %s\n",
					p->mynum,xgp->progname,p->iosize,dioinfo.d_maxiosz,target_name);
				fflush(xgp->errout);
				return(-1);
			}
			/* If the iosize is not a multiple of the alignment
			 * then exit 
			 */
			if ((p->iosize % dioinfo.d_miniosz) != 0) {
fprintf(xgp->errout,"(%d) %s: The iosize of %d bytes is not an integer mutiple of the DIRECTIO iosize <%d bytes> for target %s\n",
					p->mynum,xgp->progname,p->iosize,dioinfo.d_miniosz,target_name);
				fflush(xgp->errout);
				return(-1);
			}
		} /* end of DIO for IRIX */
#elif SOLARIS /* DIO for SOLARIS */
		i = directio(fd,DIRECTIO_ON);
		if (i < 0) {
			fprintf(xgp->errout,"(%d) %s: could not set DIRECTIO flag for: %s\n",
					p->mynum,xgp->progname,target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
#endif /* end of DIO for SOLARIS */
	} /* end of IF stmnt that opens with DIO */
#if (IRIX || ALTIX)
	/* Preallocate storage space if necessary */
	if (p->preallocate) {
		flock.l_whence = SEEK_SET;
		flock.l_start = 0;
		flock.l_len = (uint64_t)(p->preallocate * p->block_size);
		i = fcntl(fd, F_RESVSP64, &flock);
		if (i < 0) {
fprintf(xgp->errout,"(%d) %s: **WARNING** Preallocation of %lld bytes <%d blocks> failed for target %s\n",
					p->mynum,xgp->progname,(uint64_t)(p->preallocate * p->block_size),
				p->preallocate,target_name);
			fflush(xgp->errout);
			perror("Reason");
		}
	} /* end of Preallocation */
#endif /* IRIX preallocate */
#endif
#if (SNFS)
	if (p->queue_depth > 1) { /* Set this SNFS flag so that we can do parallel writes to a file */
		status = fcntl(fd, F_CVSETCONCWRITE);
		if (status < 0) {
			fprintf(xgp->errout,"(%d) %s: could not set SNFS flag F_CVSETCONCWRITE for: %s\n",
				p->mynum,xgp->progname,target_name);
			fflush(xgp->errout);
			perror("reason");
		} else {
			if (xgp->global_options & RX_VERBOSE) {
				fprintf(xgp->output,"(%d) %s: SNFS flag F_CVSETCONCWRITE set for parallel I/O for: %s\n",
					p->mynum,xgp->progname,target_name);
				fflush(xgp->output);
			}
		}
	}
#endif
	return(fd);
#endif /* Unix open stuff */  
} /* end of xdd_open_target() */

