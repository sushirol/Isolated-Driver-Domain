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
 * This file contains the subroutines necessary to display results.
 */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_display_results_header() -  simply display the header before the results
 */
void
xdd_display_results_header(void) {

	fprintf(xgp->output, "                     T  Q       Bytes      Ops    Time      Rate      IOPS   Latency     %%CPU  OP_Type    ReqSize     ");

	if (xgp->global_options & (RX_CREATE_NEW_FILES | RX_REOPEN | RX_RECREATE)) {
		fprintf(xgp->output, "OpenTime_usec");
	}
	fprintf(xgp->output, "\n");

        if (xgp->global_options & RX_CSV) {
		fprintf(xgp->csvoutput, ",Target,Q,Bytes,Ops,Time,Avg Rate,IOPS,Latency,Percent CPU,OP_Type,ReqSize");
	        if (xgp->global_options & (RX_CREATE_NEW_FILES | RX_REOPEN | RX_RECREATE)) {
		     fprintf(xgp->csvoutput, ",OpenTime_usec");
	        }
	        fprintf(xgp->csvoutput, "\n");
        }

} /* end of xdd_display_results_header() */

/*----------------------------------------------------------------------------*/
/* xdd_calculate_rates() -  
 */
void
xdd_calculate_rates(results_t *results) {
	/* Calculate combined rates */
	if ((results->elapsed > 0.0) && (results->ops > 0)) {
  		results->avg_rate = (results->bytes/(results->elapsed))/1000000.0;
  		results->avg_iops = (results->ops / results->elapsed);
  		results->avg_latency = (results->elapsed / results->ops);
		results->reqsize = (results->bytes / results->ops);
	} else {
  		results->avg_rate = -1;
  		results->avg_iops = -1;
		results->avg_latency = -1;
		results->reqsize = -1;
	}
	if (results->deskew_window_time > 0.0) {
		results->deskewed_rate = (results->deskew_window_bytes/results->deskew_window_time)/1000000.0;
	}
} /* end of xdd_calculate_rates() */
/*----------------------------------------------------------------------------*/
/* xdd_init_results() - Initialize the results structure 
 */
void
xdd_init_results(results_t *results) {
	results->errors = 0;                /* Number of I/O errors */
	results->reqsize = 0;               /* request size in bytes */
	results->passes_completed = 0;      /* Number of passes completed for this set of results */
	strcpy(results->optype, "nop");     /* default to no-operation */
	results->avg_rate = 0.0;            /* Average data rate in 10^6 bytes/sec */
	results->hi_rate = -1.0;            /* Highest data rate in 10^6 bytes/sec */
	results->low_rate = 1000000000.0;   /* Lowest data rate in 10^6 bytes/sec */
	results->deskewed_rate = 0.0;       /* Average data rate in 10^6 bytes/sec */
	results->hi_elapsed = -1.0;         /* Highest elapsed time in seconds */
	results->low_elapsed = 1000000000.0;/* Lowest elapsed time in seconds */
	results->elapsed = 0.0;             /* Total elapsed time in seconds */
	results->deskew_window_time = 0.0;  /* Front end skew elapsed time in seconds */
	results->frontend_skew_time = 0.0;  /* Front end skew elapsed time in seconds */
	results->backend_skew_time = 0.0;   /* Back end skew elapsed time in seconds */
	results->avg_latency = 0.0;         /* Average latency time in seconds */
	results->hi_latency = -1.0;         /* Highest latency time in seconds */
	results->low_latency = 100000000.0; /* Lowest latency time in seconds */
	results->bytes = 0;                 /* Total bytes transfered */
	results->deskew_window_bytes = 0;   /* Total bytes transfered during deskew period */
	results->frontend_skew_bytes = 0;   /* Total bytes transfered during frontend skew period */
	results->backend_skew_bytes = 0;    /* Total bytes transfered during backend skew period */
	results->ops = 0;                   /* Total operations performed */
	results->compare_errors = 0;        /* Total number of errors on data sequenced compare check */
	results->avg_iops = 0.0;            /* Average number of operations per second */
	results->user_time = 0.0;           /* Amount of CPU time used by the application */
	results->system_time = 0.0;         /* Amount of CPU time used by the system */
	results->us_time = 0.0;             /* Total CPU time used by this process: user+system time */
	results->percent_cpu = 0.0;         /* Percent of CPU used by this process */
	results->open_start_time = 0;       /* time stamp just before openning target */
	results->open_end_time = 0;         /* time stamp just after openning target */
	results->open_time = 0;             /* Open time stamp */
	results->start_time = LONGLONG_MIN; /* Start time stamp */
	results->end_time = 0;              /* end time */
} /* end of xdd_init_results() */
/*----------------------------------------------------------------------------*/
/* xdd_combine_results() - Combine results of the "from" results and put them
 * in the "to" results structure.
 */
void
xdd_combine_results(results_t *from, results_t *to) { 
	/* Collect all the results from the thread results areas */
	to->bytes        += from->bytes;
	to->ops          += from->ops;
	if (to->ops > 0)
		to->reqsize = to->bytes / to->ops;
	else to->reqsize = from->reqsize;
	to->compare_errors  += from->compare_errors;
	to->errors       += from->errors;
	to->user_time   += from->user_time;
	to->system_time  += from->system_time;
	to->us_time   += from->us_time;
	to->percent_cpu  += from->percent_cpu;
	to->deskew_window_bytes += from->deskew_window_bytes;
	to->frontend_skew_bytes += from->frontend_skew_bytes;
	to->backend_skew_bytes  += from->backend_skew_bytes;
	to->deskew_window_time  += from->deskew_window_time;
	/* Highest Elapsed time is the "combined" value */
	if (from->elapsed > to->elapsed) 
		to->elapsed = from->elapsed;
	/* Highest rate of all threads is the "combined" highest rate */
	if (from->hi_rate > to->hi_rate) {
      	to->hi_rate = from->hi_rate;
      	to->low_elapsed = from->low_elapsed;
    }
	/* Lowest rate of all threads is the "combined" Lowest rate */
    if (from->low_rate < to->low_rate) {
      	to->low_rate = from->low_rate;
      	to->hi_elapsed = from->hi_elapsed;
    }
	/* Highest elapsed time of all threads is the "combined" highest elapsed time */
	if (from->hi_elapsed > to->hi_elapsed) {
      	to->hi_elapsed = from->hi_elapsed;
    }
	/* Lowest elapsed time of all threads is the "combined" Lowest elapsed time */
    if (from->low_elapsed < to->low_elapsed) {
      	to->low_elapsed = from->low_elapsed;
    }
	/* Get the earliest start time */
	if (from->start_time < to->start_time)
		to->start_time = from->start_time;
	if (from->end_time > to->end_time)
		to->end_time = from->end_time;

	/* Get the OPEN time for this target */
	to->open_time += from->open_time;
	to->passes_completed = from->passes_completed;

	if (strcmp(to->optype, "nop") == 0){ // This is the first time through so set the "to" optype to whatever is in the "from" optype
		strcpy(to->optype, from->optype);
	} else { // The "to" optype must already be something so let's see if it is the same or different than the "from" optype
		if ((strcmp(to->optype, "read") == 0) && (strcmp(from->optype, "write") == 0)) // Make the optype mixed
			strcpy(to->optype, "mixed");
		if ((strcmp(to->optype, "write") == 0) && (strcmp(from->optype, "read") == 0)) // Make the optype mixed
			strcpy(to->optype, "mixed");
	}

	/* Calculate combined rates */
	xdd_calculate_rates(to);
} /* end of xdd_combine_results() */
/*----------------------------------------------------------------------------*/
/* xdd_display_results() - Display results on the output device
 * The "id" is the output line identification string such as
 * "Average" or "Total" for per-pass intermediate results or
 * "Combined" for the combined results of all threads.
 * The second argument to this routine points to the results structure
 * containing all the data do display.
 * The last argument is the output file pointer. The results can
 * be sent to multiple destinations.
 */
void
xdd_display_results(char *id, int32_t target, int32_t queue, int64_t bytes, int64_t ops, double time, double rate, double iops, double latency, double percentcpu, char *optype, int64_t reqsize, pclk_t open_time, pclk_t start_time, pclk_t end_time, FILE *ofp, FILE *cfp, uint64_t verbosity) {
	fprintf(ofp,
//ID   T   Q    Bytes    Ops      Time    Rate      IOPS    Latency  %CPU    OP_Type      ReqSize    <OpenTime>
#ifdef WIN32
"\r%s %4d %2d  %12I64d %6I64d %7.3f   %7.3f    %6.2f   %7.4f    %5.2f   %s  %10I64d ",
#else
"\r%s %4d %2d  %12lld   %6lld   %7.3f   %7.3f     %6.2f   %7.4f    %5.2f   %s  %10lld " ,
#endif
	id,target,queue,bytes,ops,time,rate,iops,latency,percentcpu,optype,reqsize);

	// Check to see if we need to display the open time 
        if (xgp->global_options & (RX_CREATE_NEW_FILES | RX_REOPEN | RX_RECREATE)) { // display the open time too
		fprintf(ofp,
#ifdef WIN32
"%10I64d ",
#else
"%10lld ",
#endif
		open_time);
	}

	// Check to see if we need to display the start and end time in pico seconds - only for real geeks
	if (verbosity & RX_REALLYVERBOSE) {
		fprintf(ofp,
#ifdef WIN32
"%I64u   %I64u ",
#else
"%llu   %llu ",
#endif
		start_time, end_time);
	}
	fprintf(ofp,"\n");
	fflush(ofp);
	if (cfp) 
		xdd_display_csv_results(id,target,queue,bytes,ops,time, rate, iops, latency, percentcpu, optype, reqsize, open_time, start_time, end_time, cfp, verbosity);
} /* end of xdd_display_results() */
/*----------------------------------------------------------------------------*/
/* xdd_display_csv_results() - Display results on the output device as comma-separated-values
 * The "id" is the output line identification string such as
 * "Average" or "Total" for per-pass intermediate results or
 * "Combined" for the combined results of all threads.
 * The second argument to this routine points to the results structure
 * containing all the data do display.
 * The last argument is the output file pointer. The results can
 * be sent to multiple destinations.
 */
void
xdd_display_csv_results(char *id, int32_t target, int32_t queue, int64_t bytes, int64_t ops, double time, double rate, double iops, double latency, double percentcpu, char *optype, int64_t reqsize, pclk_t open_time, pclk_t start_time, pclk_t end_time, FILE *cfp, uint64_t verbosity) {
	fprintf(cfp,
//ID   T   Q    Bytes    Ops      Time    Rate      IOPS    Latency  %CPU <StartTime EndTime>
#ifdef WIN32
"%s,%4d,%2d,%12I64d,%6I64d,%7.3f,%7.3f,%6.2f,%7.4f,%5.2f,%s,%10I64d",
#else
"%s,%4d,%2d,%12lld,%6lld,%7.3f,%7.3f,%6.2f,%7.4f,%5.2f,%s,%10lld",
#endif
	id,target,queue,bytes,ops,time,rate,iops,latency,percentcpu,optype,reqsize);

	// Check to see if we need to display the open time 
        if (xgp->global_options & (RX_CREATE_NEW_FILES | RX_REOPEN | RX_RECREATE)) { // display the open time too
		fprintf(cfp,
#ifdef WIN32
",%10I64d",
#else
",%10lld",
#endif
		open_time);
	}

	// Check to see if we need to display the start and end time in pico seconds - only for real geeks
	if (verbosity & RX_REALLYVERBOSE) {
		fprintf(cfp,
#ifdef WIN32
",%I64u,%I64u",
#else
",%llu,%llu",
#endif
		start_time, end_time);
	}
	fprintf(cfp,"\n");
	fflush(cfp);
} /* end of xdd_display_csv_results() */
/*----------------------------------------------------------------------------*/
/* xdd_display_qthread_pass_results() - This will display the results for a 
 * specific qthread pointed to by the ptds pointer passed in.
 */
void
xdd_display_qthread_pass_results(ptds_t *p, FILE *ofp, FILE *cfp, int32_t verbosity) {
	char   pass_string[128];
	/* Calculate combined rates */
	xdd_calculate_rates(&p->perpass);
	if (verbosity & RX_QTHREAD_INFO) {
		sprintf(pass_string,"QUEUE    PASS%04d",p->current_pass);
		xdd_display_results(
			pass_string,  // id
			p->mynum,   // target 
			p->myqnum,   // queue
			p->perpass.bytes, // bytes, 
			p->perpass.ops,  // ops, 
			p->perpass.elapsed, //time, 
			p->perpass.avg_rate, // rate, 
			p->perpass.avg_iops, // iops 
			p->perpass.avg_latency, // latency, 
			p->perpass.percent_cpu, // percentcpu,
			p->perpass.optype, // Operation Type
			p->perpass.reqsize, // Average Request Size
			(pclk_t)(0), // Open time - not applicable in this case
			(pclk_t)(p->perpass.start_time - p->base_time), // Start time for this queue thread
			(pclk_t)(p->perpass.end_time - p->base_time), // End time for this queue thread
			ofp,cfp, verbosity);
	}
} /* end of xdd_display_qthread_pass_results() */
/*----------------------------------------------------------------------------*/
/* xdd_display_Target_pass_results() - This will gather the results from all
 * qthreads for a specific target and display the combined result. It is assumed
 * that the ptds pointer passed in points to qthread 0 of the associated target.
 */
void
xdd_display_target_pass_results(ptds_t *p, FILE *ofp, FILE *cfp, int32_t verbosity) {
	results_t  combined; /* Combined Results */
	ptds_t   *qp;  /* QThread ptds Pointer */
	char   pass_string[32];
	/* Init this results area */
	xdd_init_results(&combined);
	/* Collect all the results from the associated qthreads and combine them into a single results structure */
	qp = p;
	while (qp) {
		xdd_combine_results(&qp->perpass, &combined);
		qp = qp->nextp;
  	} /* end of WHILE loop */
	/* Calculate combined rates */
	xdd_calculate_rates(&combined);
	if (verbosity & RX_VERBOSE) {
		sprintf(pass_string,"TARGET   PASS%04d",p->current_pass);
		xdd_display_results(
			pass_string,  // id
			p->mynum,   // target 
			p->queue_depth,  //queue
			combined.bytes,  // bytes, 
			combined.ops,  // ops, 
			combined.elapsed, //time, 
			combined.avg_rate, // rate, 
			combined.avg_iops, // iops 
			combined.avg_latency, // latency, 
			combined.percent_cpu, // percentcpu,
			combined.optype, // Operation Type
			combined.reqsize, // Average Request Size
			(pclk_t)(combined.open_time+500000)/1000000, // Average open time
			(pclk_t)(combined.start_time - p->base_time),
			(pclk_t)(combined.end_time - p->base_time),
			ofp,cfp, verbosity);
	}
} /* end of xdd_display_Target_pass_results() */
/*----------------------------------------------------------------------------*/
/* xdd_display_target_average_results() - This will gather the results from all
 * qthreads and display the combined result. It is assumed
 * that the ptds pointer passed in points to qthread 0 of target 0.
 */
void
xdd_display_target_average_results(ptds_t *p, FILE *ofp, FILE *cfp, int32_t verbosity) {
	ptds_t   *qp;  /* QThread ptds Pointer */
	/* Init this results area */
	xdd_init_results(&p->target_results);
	/* Collect all the results from the associated qthreads and combine them into a single results structure */
	qp = p;
	while (qp) {
		xdd_combine_results(&qp->qthread_results, &p->target_results);
		qp = qp->nextp;
  	} /* end of WHILE loop */
	xdd_calculate_rates(&p->target_results);
	if (p->target_results.passes_completed) {
		p->target_results.open_time /= p->target_results.passes_completed;
	}
	else {
		p->target_results.open_time = 0;
	}

	if (verbosity & RX_VERBOSE) {
		xdd_display_results(
			"TARGET   Average ",  // id
			p->mynum,   // target 
			p->queue_depth,  //queue
			p->target_results.bytes,  // bytes, 
			p->target_results.ops,  // ops, 
			p->target_results.elapsed, //time, 
			p->target_results.avg_rate, // rate, 
			p->target_results.avg_iops, // iops 
			p->target_results.avg_latency, // latency, 
			p->target_results.percent_cpu, // percentcpu 
			p->target_results.optype, // Operation Type 
			p->target_results.reqsize, // Average Request Size
			(pclk_t)(p->target_results.open_time+500000)/1000000, // Average open time
			(pclk_t)(p->target_results.start_time - p->base_time),
			(pclk_t)(p->target_results.end_time - p->base_time),
			ofp,cfp, verbosity);
	}
} /* end of xdd_display_target_average_results() */
/*----------------------------------------------------------------------------*/
/* xdd_display_qthread_average_results() - This will display the results for a 
 * specific qthread pointed to by the ptds pointer passed in.
 */
void
xdd_display_qthread_average_results(ptds_t *p, FILE *ofp, FILE *cfp, int32_t verbosity) {
	/* Calculate combined rates */
	xdd_calculate_rates(&p->qthread_results);
	p->qthread_results.percent_cpu = (p->qthread_results.us_time / p->qthread_results.elapsed) * 100.0;
	if (verbosity & RX_QTHREAD_INFO) {
		xdd_display_results(
			"QUEUE    Average ",  // id
			p->mynum,   // target 
			p->myqnum,   // queue
			p->qthread_results.bytes, // bytes, 
			p->qthread_results.ops,  // ops, 
			p->qthread_results.elapsed, //time, 
			p->qthread_results.avg_rate, // rate, 
			p->qthread_results.avg_iops, // iops 
			p->qthread_results.avg_latency, // latency, 
			p->qthread_results.percent_cpu, // percentcpu 
			p->qthread_results.optype, // Operation Type 
			p->qthread_results.reqsize, // Average Request Size
			(pclk_t)(p->qthread_results.open_time+500000)/1000000, // Average open time
			(pclk_t)(p->qthread_results.start_time - p->base_time),
			(pclk_t)(p->qthread_results.end_time - p->base_time),
			ofp,cfp, verbosity);
	}
} /* end of xdd_display_qthread_average_results() */

/*----------------------------------------------------------------------------*/
/* xdd_deskew() - Calculate deskew results 
 * Ending status: 0=goodness, -1=bad juju
 */
int32_t
xdd_deskew(void) {
	int32_t  i,j;
	pclk_t  est;    /* Earliest Start Time */
	pclk_t  nest;    /* Normalized Earliest Start Time */
	double  fest;    /* Earliest Start Time */
	int32_t  est_target;   /* target number that had the Earliest Start Time */
	int32_t  est_qthread;  /* qthread number that had the Earliest Start Time */
	pclk_t  eet;    /* Earliest End Time */ 
	pclk_t  neet;    /* Normalized Earliest End Time */ 
	double  feet;    /* Earliest End Time */ 
	int32_t  eet_target;   /* target number that had the Earliest End Time */
	int32_t  eet_qthread;  /* qthread number that had the Earliest End Time */
	pclk_t  lst;    /* Latest Start Time */
	pclk_t  nlst;    /* Normalized Latest Start Time */
	double  flst;    /* Latest Start Time */
	int32_t  lst_target;   /* target number that had the Latest Start Time */
	int32_t  lst_qthread;  /* qthread number that had the Latest Start Time */
	pclk_t  let;    /* Latest End Time */
	pclk_t  nlet;    /* Normalized Latest End Time */
	double  flet;    /* Latest End Time */
	int32_t  let_target;   /* target number that had the latest End Time */
	int32_t  let_qthread;  /* qthread number that had the latest End Time */
	pclk_t  base_time;   /* Easliest start time - used to normalize the other times */
	pclk_t  deskew_window_time; /* The time between lst and eet in clock ticks (tiny ones) */
	double  frontend_skew_time; /* Frontend skew in seconds - time between est and lst */
	double  fdeskew_window_time;/* The time between lst and eet in seconds */
	double  backend_skew_time; /* Backend skew in seconds -  time between eet and let in seconds */
	ptds_t  *qp, *tp;   /* Pointers to the target and qthread ptds */
	tthdr_t  *thp;    /* Pointer to the timestamp table header */
	tte_t  *tep;    /* pointer to a timestamp table entry */
	double  delta;    /* amount of time between start of an operation and LST */
	double  iotime;    /* amount of time for the io being examined */
	uint64_t deskew_window_bytes;/* Total number of bytes transferred during the deskew window */
	double  deskew_rate;  /* The deskewed data rate */
	char  pass_string[128];


	/* Init the target and qthread values to 0 */
	est_target = 0;
	est_qthread = 0;
	eet_target = 0;
	eet_qthread = 0;
	lst_target = 0;
	lst_qthread = 0;
	let_target = 0;
	let_qthread = 0;
	eet = let = xgp->ptds[0].my_end_time;
	est = lst = xgp->ptds[0].my_start_time;
	/* Go through all the target threads and associated qthreads and find the 
	* last thread to start and the first thread to end. */
	for (i=0; i<xgp->number_of_targets; i++) {
		tp = &xgp->ptds[i]; /* Current target pointer */
		qp = tp; /* Qthread pointer */
		while (qp) { /* Look at each qthread for this target */
			if (qp->my_start_time < est) { /* Check for earliest start time (est)*/
				est = qp->my_start_time;
				est_target = qp->mynum;
				est_qthread = qp->myqnum;
			}
			if (qp->my_end_time < eet) { /* Check for Earliest End time */
				eet = qp->my_end_time;
				eet_target = qp->mynum;
				eet_qthread = qp->myqnum;
			}
			if (qp->my_start_time > lst) { /* Check for the latest start time */
				lst = qp->my_start_time;
				lst_target = qp->mynum;
				lst_qthread = qp->myqnum;
			}
			if (qp->my_end_time > let) { /* Check for Latest End Time */
				let = qp->my_end_time;
				let_target = qp->mynum;
				let_qthread = qp->myqnum;
			}
			qp = qp->nextp;
		} /* end of WHILE loop */
	} /* end of FOR loop */
	/* Sanity check */
	if (lst >= eet ) { /* Houston, we have a problem. The latest start time happened after a target finished */
		fprintf(xgp->errout, "DESKEW info: The skew for this run is too high to make a reasonable calculation\n");
		return(-1);
	}
	/* Normailze the earliest/latest start/end times */
	base_time = est;
	nest = est - base_time;
	neet = eet - base_time;
	nlst = lst - base_time;
	nlet = let - base_time;
	fest = (double)(nest/1000000000000.0);
	feet = (double)(neet/1000000000000.0);
	flst = (double)(nlst/1000000000000.0);
	flet = (double)(nlet/1000000000000.0);
	/* At this point we know the number of the target and qthread that was the last to start and first to end as well
	 * as the start and end time values. */
	if (xgp->global_options & RX_REALLYVERBOSE) {
		fprintf(xgp->output, "DESKEW info: Earliest Target/Qthread to start was %d/%d at time, %10.6f, seconds\n",est_target, est_qthread,fest);
		fprintf(xgp->output, "DESKEW info: Earliest Target/Qthread to  end  was %d/%d at time, %10.6f, seconds\n",eet_target, eet_qthread,feet);
		fprintf(xgp->output, "DESKEW info: Latest Target/Qthread to start was   %d/%d at time, %10.6f, seconds\n",lst_target, lst_qthread,flst);
		fprintf(xgp->output, "DESKEW info: Latest Target/Qthread to  end  was   %d/%d at time, %10.6f, seconds\n",let_target, let_qthread,flet);
	}
	/* Now we can calculate the deskew "window" which is the timeframe when all targets are transferring data.
	 * This is essentially after the latest start time and the earliest end time.
	 */
	deskew_window_time = eet - lst;
	fdeskew_window_time = (double)((deskew_window_time) / 1000000000000.0);
	frontend_skew_time = (double)((lst-est)/1000000000000.0);
	backend_skew_time = (double)((let-eet)/1000000000000.0);
	/* Display the frontend skew, backend skew, and deskewed window */
	if (xgp->global_options & RX_REALLYVERBOSE) {
		fprintf(xgp->output, "DESKEW info: Frontend skew is %10.6f seconds\n",frontend_skew_time);
		fprintf(xgp->output, "DESKEW info: deskewed window is %10.6f seconds\n",fdeskew_window_time);
		fprintf(xgp->output, "DESKEW info: Backend skew is %10.6f seconds\n",backend_skew_time);
		fprintf(xgp->output, "DESKEW info: Total time window is %10.6f seconds\n",flet);
	}
	/* Now we need to look at all the timestamp buffers to find out how much data was transferred 
	 * during the frontend skew period and during the backend skew period. */
	deskew_window_bytes = 0;
	for (i=0; i<xgp->number_of_targets; i++) {
		tp = &xgp->ptds[i]; /* Current target pointer */
		qp = tp; /* Qthread pointer */
		while (qp) { /* Look at each qthread for this target */
			/* Get the timestamp buffer pointer for this T/Q */
			thp = qp->ttp;
			if (thp == 0) {
				fprintf(xgp->output,"DESKEW error: no timestamp table available to calculate frontend/backend skew data\n");
				return(-1);
			}
			if ((qp->mynum == lst_target) && (qp->myqnum == lst_qthread)) {
				/* This is the latest start time thread so there is no frontend skew */
				qp->front_end_skew_bytes = 0;
			} else { /* Calculate the front_end_skew_bytes from the time stamp table */
				if (qp->timestamps == 0) {
					fprintf(xgp->output,"DESKEW error: no timestamp entries available to calculate frontend/backend skew data\n");
					return(-1);
				}
				/* Scan the timestamp table looking for the first end-time beyond LST */
				tep = &thp->tte[0];
				qp->front_end_skew_bytes = 0;
				for (j=0; j < qp->timestamps; j++, tep++) { 
					/* look through these entries and count just the ones that occur before latest start time. 
					* All values are pclk_t types */
					if (tep->end <= lst) {
						qp->front_end_skew_bytes += qp->iosize;
					} else if ((tep->start < lst) && (tep->end > lst)) {
						/* This operation straddles the latest start time.
						* Therefore we take only a percentage of the bytes transferred. */
						delta = (lst - tep->start)/1000000000000.0;
						iotime = (tep->end - tep->start)/1000000000000.0;
						qp->front_end_skew_bytes += ((delta/iotime) * qp->iosize);
					} else break;
				}
			} /* Done calculating front_end_skew_bytes */
			if ((qp->mynum == eet_target) && (qp->myqnum == eet_qthread)) {
				/* This is the latest start time thread so there is no frontend skew */
				qp->back_end_skew_bytes = 0;
			} else { /* Calculate the back_end_skew_bytes */
				delta = (qp->my_end_time - eet)/1000000000000.0;
				iotime = (qp->my_end_time - qp->my_current_start_time)/1000000000000.0;
				if (iotime > 0)
					qp->back_end_skew_bytes = ((delta/iotime) * qp->iosize);
				else qp->back_end_skew_bytes = 0;
			} /* Done calculating back_end_skew_bytes */
			/* Calculate the total number of bytes actually transferred during the deskew window */
			qp->deskew_window_bytes = qp->perpass.bytes - qp->front_end_skew_bytes - qp->back_end_skew_bytes;
			deskew_window_bytes += qp->deskew_window_bytes;
			if (qp->current_pass < xgp->passes) { /* Do not reset the timestamp table if this is the last pass */
				/* Reset the timestamp table information for the next pass */
				thp->tte_indx = 0;
				qp->ts_options |= TS_ON;
				qp->timestamps = 0;
				/* clear everything out of the trace table */
				memset(&thp->tte[0],(thp->tt_size*sizeof(tte_t)), 0);
			}
			qp = qp->nextp;
		} /* end of WHILE loop */
	} /* end of FOR loop */
	if (xgp->global_options & RX_REALLYVERBOSE) {
#if WIN32
fprintf(xgp->output,"DESKEW Total bytes transferred during the deskew period %I64d\n",deskew_window_bytes);
#else
fprintf(xgp->output,"DESKEW Total bytes transferred during the deskew period %lld\n",deskew_window_bytes);
#endif
	}
	if (fdeskew_window_time > 0.0) 
		deskew_rate = (double)((deskew_window_bytes / fdeskew_window_time) / 1000000.0);
	else deskew_rate = 0;
	/* reset the deskew_ring alarm for the next pass */
	xgp->deskew_ring = 0;
	sprintf(pass_string,"DESKEWED PASS%04d",xgp->ptds[0].current_pass);
	xdd_display_results(
			pass_string,  // id
			xgp->ptds[0].total_threads, // target 
			0,  //queue
			deskew_window_bytes,  // bytes, 
			0,  // ops, 
			fdeskew_window_time, //time, 
			deskew_rate, // rate, 
			0, // iops 
			0, // latency, 
			0, // percentcpu,
			"NA", // Optype
			0, // Average Request Size
			(pclk_t) (0), // NA
			(pclk_t)(lst),
			(pclk_t)(eet),
			xgp->output,xgp->csvoutput, xgp->global_options&(RX_VERBOSE|RX_REALLYVERBOSE));
	xgp->deskew_total_rates += deskew_rate;
	xgp->deskew_total_bytes += deskew_window_bytes;
	xgp->deskew_total_time += fdeskew_window_time;
	deskew_rate = (double)((xgp->deskew_total_bytes / xgp->deskew_total_time) / 1000000.0);
	sprintf(pass_string,"DSAVG    PASS%04d",xgp->ptds[0].current_pass);
	xdd_display_results(
			pass_string,  // id
			xgp->ptds[0].total_threads, // target 
			0,  //queue
			xgp->deskew_total_bytes,  // bytes, 
			0,  // ops, 
			xgp->deskew_total_time, //time, 
			deskew_rate, // rate, 
			0, // iops 
			0, // latency, 
			0, // percentcpu, 
			"NA", // Operation Type 
			0, // Average Request Size
			(pclk_t) (0), // NA
			(pclk_t)(lst),
			(pclk_t)(eet),
			xgp->output,xgp->csvoutput, xgp->global_options&(RX_VERBOSE|RX_REALLYVERBOSE));
	return(0);
} /* End of xdd_deskew() */
