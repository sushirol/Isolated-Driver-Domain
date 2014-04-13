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
 * This file contains the subroutines necessary to generate the seeklist 
 * which has the implied access pattern.
 */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* xdd_init_seek_list() - Generate the list of seek operations to perform
 * This routine will generate a list of locations to access within the
 * specified range for random seeks or within the implied range for 
 * purely sequential operations. 
 * The seek list is either loaded from a specified file or is generated
 * by this routine. 
 * Each entry in the seek list contains the seek location, the size of the
 * data transfer (currently reqsize), and the operation to perform.
 * The seek entries are first loaded with their locations and a second
 * pass assigns operations (read or write) to the locations as
 * necessary. 
 * Example A - A normal 100% write seek list
 *
 * Operation# Location Op 
 *    0     0  W 
 *    1     1024  W 
 *    2     2048  W 
 *    3     3072  W 
 *    n     n*1024 W
 *
 */
void
xdd_init_seek_list(ptds_t *p) {
	int32_t  j;
	int32_t  rw_op_index;  /* relative rw index from 0 to the number of rw ops minus 1  */
	int32_t  rw_index;   /* absolute index into seek list for the current rw op*/
	int32_t  rw_index_incr;  /* read/write index increment value (1 or 2) */
	int32_t  op_index;   /* Current operation number  (from 0 to sp->seek_total_ops-1 ) */
	int64_t  gap;    /* The gap in blocks between staggered locations */
	int64_t  interleave_threadoffset;
	double  bytes_per_sec;  /* The tranfer rate requested by the -throttle option */
	double  seconds_per_op; /* a floating point representation of the time per operation */
	double  variance_seconds_per_op; /* a floating point representation of the time variance per operation */
	double  seconds_per_op_low; /* a floating point representation of the time per operation */
	double  seconds_per_op_high; /* a floating point representation of the time per operation */
    double  low_bw, hi_bw;
	double  bytes_per_request; /* self explanatory */
	pclk_t  pico_seconds_per_op; /* self explanatory */
    pclk_t  pico_second_throttle_variance; /* Max variance per operation */
	pclk_t  relative_time; /* Time in picosecond relative to the first operation */
	int32_t  previous_percent_op; /* used to determine read/write operation */
	int32_t  percent_op;  /* used to determine read/write operation */
	int32_t  current_op;  /* Current operation - RX_READ or RX_WRITE */
	char  state[256];
	seekhdr_t *sp;   /* pointer to the seek header */
	/* If a throttle value has been specified, calculate the time that each operation should take */
	if (p->throttle > 0.0){
		if (p->throttle_type & RX_THROTTLE_BW){
			bytes_per_sec = p->throttle * MILLION;
			bytes_per_request = (p->reqsize * p->block_size);
			seconds_per_op = bytes_per_request/bytes_per_sec;
			pico_seconds_per_op = seconds_per_op * TRILLION;
            if (p->throttle_variance) {
                low_bw = (p->throttle - p->throttle_variance) * MILLION;
                seconds_per_op_high = bytes_per_request / low_bw;
                hi_bw = (p->throttle + p->throttle_variance) * MILLION;
                seconds_per_op_low = bytes_per_request / hi_bw;
                pico_second_throttle_variance = seconds_per_op - (bytes_per_request / low_bw);
            } else {
                low_bw = bytes_per_sec;
                hi_bw = bytes_per_sec;
            }
		} else if (p->throttle_type & RX_THROTTLE_OPS){
			seconds_per_op = 1.0 / p->throttle;
			pico_seconds_per_op = seconds_per_op * TRILLION;
            variance_seconds_per_op = 1.0 / p->throttle_variance;
            pico_second_throttle_variance = variance_seconds_per_op * TRILLION;
		}
	} else pico_seconds_per_op = 0;
	sp = &p->seekhdr;
	/* Initialize the random number generator */
	initstate(sp->seek_seed, state, 256);
	/* Check to see if we need to load the seeks from a specified file */
	if (sp->seek_options & RX_SEEK_LOAD) { /* Load pre-defined seek list */
		xdd_load_seek_list(p);
		sp->seek_options &= ~RX_SEEK_LOAD; /* only want to load seek list once */
	} else { /* Generate a new seek list */ 
		relative_time = pico_seconds_per_op + p->start_delay;
		rw_op_index = 0;
		rw_index = 0;
		rw_index_incr = 1;
		sp->seek_num_rw_ops = sp->seek_total_ops;
		if (p->rwratio >= 0.5) /* This has to be set correctly or the first op may not be correct */
			previous_percent_op = -1.0;
		else previous_percent_op = 0.0;
		for (op_index = 0; op_index < sp->seek_total_ops; op_index++) {   
			/* Fill in the seek location */
			if (sp->seek_options & RX_SEEK_RANDOM) { /* generate a random seek location */
				if (rw_op_index == 0) { /* This is the first location for this thread */
					for (j = 0; j < p->myqnum; j++) /* skip over the first N locations */
						sp->seeks[rw_index].block_location = (uint64_t)(sp->seek_range * xdd_random_float());
					/* Assign this location as the first seek */
					sp->seeks[rw_index].block_location = (uint64_t)(sp->seek_range * xdd_random_float());
				} else { /* This section if for seek locations 2 thru N */
					/* This is done to support interleaved I/O operations and/or command queuing */
					for (j = 0; j < sp->seek_interleave; j++)
						sp->seeks[rw_index].block_location = (uint64_t)(sp->seek_range * xdd_random_float());
				}
			} else {/* generate a sequential seek */
				if (sp->seek_options & RX_SEEK_STAGGER)
					gap = ((sp->seek_range-p->reqsize) - (sp->seek_num_rw_ops*p->reqsize)) / (sp->seek_num_rw_ops-1);
				else gap = 0;
				if (sp->seek_interleave > 1)
					interleave_threadoffset = (p->myqnum%sp->seek_interleave)*p->reqsize;
				else interleave_threadoffset = 0;
				sp->seeks[rw_index].block_location = p->start_offset + interleave_threadoffset + 
						(rw_op_index * ((p->reqsize*sp->seek_interleave)+gap));
			} /* end of generating a sequential seek */
			/* Now lets fill in the request sizes to transfer */
			sp->seeks[rw_index].reqsize = p->reqsize;
			/* Now lets fill in the appropriate operation */
			/* The operation is specified either as "read" or "write" in which case
			 * all operations for this target will be either read or write accordingly.
			 * The way this is actually done is that when the command line arguments are
			 * parsed, if the -op read or -op write options are specified then the
			 * rwratio is set to 100 or 0 accordingly. This way, the operation is
			 * determined soley by the rwratio parameter. 
			 * If the "rwratio" was specified, then the appropriate number
			 * of read and write operations are used. 
			 * The -rwratio option takes precedence over the -op option.
			 */
			percent_op = p->rwratio * rw_op_index;
			if (percent_op > previous_percent_op) 
				current_op = RX_READ;
			else current_op = RX_WRITE;
			previous_percent_op = percent_op;

            /* Fill in the operation */
			if (current_op == RX_WRITE) { /* This is a WRITE operation */
				sp->seeks[rw_index].operation = RX_WRITE;
			} else { /* This is a READ operation */
				sp->seeks[rw_index].operation = RX_READ;
			}

			/* fill in the time that this operation is supposed to take place */
            //  -----------------L=========^=========H------------>
            //   Time--->        |         |         |Relative time plus the variance
            //                   |         |Relative time Average
            //                   |Relative time minus the variance
            // The actual time that an I/O operation should take place is somewhere between
            // the relative time plus or minus the variance. In Theory. Maybe.
            if (p->throttle_variance) {
                variance_seconds_per_op = ((seconds_per_op_high-seconds_per_op_low) * xdd_random_float()) * TRILLION;
                sp->seeks[rw_index].time1 = (relative_time - pico_second_throttle_variance) + variance_seconds_per_op;

            } else {
			    sp->seeks[rw_index].time1 = relative_time;

            }
			relative_time += pico_seconds_per_op;

			/* Increment to the next entry in the seek list */
			rw_index += rw_index_incr;
			rw_op_index++;
		} /* end of FOR loop */
	} /* done generating a new seek list */
	/* Save this seek list to a file if requested to do so */
	if (sp->seek_options & (RX_SEEK_SAVE | RX_SEEK_SEEKHIST | RX_SEEK_DISTHIST)) 
		xdd_save_seek_list(p);
} /* end of xdd_init_seek_list() */
/*----------------------------------------------------------------------------*/
/* xdd_save_seek_list() - save the specified seek list in a file    
 */
void
xdd_save_seek_list(ptds_t *p) {
	int32_t  i; /* working variable */
	uint64_t longest, shortest; /* Longest and shortest seek distances in blocks */
	uint64_t total, average; /* Total and average distance traveled in blocks */
	uint64_t distance; /* The distance from one location to the next */
	uint64_t bucket;  /* Index into the array of buckets */
	uint64_t *buckets; /* Pointer to the array of buckets */ 
	uint64_t divisor; /* Used to generate the histogram */
	char  *opc; /* Pointer to the operation string */
	FILE *tmp; /* FILE pointer to the file to save the seek list into */
	char errormessage[1024]; /* error message buffer */
	char tmpname[512]; /* enumerated name of the file to save the seeks into */
	seekhdr_t *sp; 
	sp = &p->seekhdr;
	sprintf(tmpname,"%s.T%dQ%d.txt",sp->seek_savefile,p->mynum,p->myqnum);
	if (sp->seek_savefile)
		tmp = fopen(tmpname,"w");
	else tmp = xgp->errout;
	if (tmp == NULL) {
		fprintf(xgp->errout,"%s: Cannot open file %s for saving seek information\n",xgp->progname,tmpname);
		perror("reason");
		return;
	}
	/* Save the seek locations in specified file */
	if (sp->seek_options & RX_SEEK_SAVE) {
		longest = 0;
		shortest = sp->seek_range;
		total = 0;
		for (i = 1; i < sp->seek_total_ops; i++) {
			if (sp->seeks[i].block_location  > sp->seeks[i-1].block_location)
				distance = sp->seeks[i].block_location - sp->seeks[i-1].block_location;
			else distance = sp->seeks[i-1].block_location - sp->seeks[i].block_location;
			if (distance > longest) longest = distance;
			if (distance < shortest) shortest = distance;
			total += distance;
		}
		average = (uint64_t)(total / sp->seek_total_ops);
		/* Print the seek list into the specified file */
#ifdef WIN32
		fprintf(tmp,"# Longest seek=%I64u, Shortest seek=%I64u, Average seek distance=%I64u\n",
#else
		fprintf(tmp,"# Longest seek=%llu, Shortest seek=%llu, Average seek distance=%llu\n",
#endif  
		longest, shortest, average);
		fprintf(tmp,"#Ordinal Location Reqsize Operation Time1 Time2\n"); 
		for (i = 0; i < sp->seek_total_ops; i++) {
			if (sp->seeks[i].operation == RX_READ) 
				opc = "r";
			else if (sp->seeks[i].operation == RX_WRITE)
				opc = "w";
			else opc = "u";
#ifdef WIN32
			fprintf(tmp,"%010d %012I64u  %d %s %016I64u %016I64u\n",
#else
			fprintf(tmp,"%010d %012llu %d %s %016llu %016llu\n",
#endif
				i,sp->seeks[i].block_location, sp->seeks[i].reqsize, opc, sp->seeks[i].time1,sp->seeks[i].time2);
		}
	} /* end of section that saves the seek locations */
	/* Collect and print any requested histogram information */
	if (sp->seek_options & RX_SEEK_SEEKHIST) { /* This section will create a seek location histogram */
		/* init the buckets and calculate the divisor */
		buckets = malloc(sp->seek_NumSeekHistBuckets * sizeof(seek_t));
		if (buckets == NULL) {
			sprintf(errormessage,"#%s: Cannot allocate %d bytes for seek histogram buckets\n",xgp->progname, sp->seek_NumSeekHistBuckets*sizeof(seek_t));
			fprintf(xgp->errout,errormessage);
			fprintf(tmp,errormessage);
		} else {
		for (i = 0; i < sp->seek_NumSeekHistBuckets; i++) buckets[i] = 0;
		divisor = sp->seek_range / sp->seek_NumSeekHistBuckets;
		if (divisor == 0) {
			sprintf(errormessage,"#%s: Cannot print Seek histogram - %d is not enough range\n",xgp->progname, sp->seek_NumSeekHistBuckets);
			fprintf(xgp->errout,errormessage);
			fprintf(tmp,errormessage);
			free(buckets);
		} else {
			/* fill the histogram buckets */
			for (i = 0; i < sp->seek_total_ops; i++) {
				bucket = sp->seeks[i].block_location/divisor;
				buckets[bucket]++;
			}
			/* print the histgram information for each bucket */
			for (i = 0; i < sp->seek_NumSeekHistBuckets; i++) {
#ifdef WIN32
				fprintf(tmp,"#SeekHist %04d %10I64u\n",
#else
				fprintf(tmp,"#SeekHist %04d %10llu\n",
#endif
					i,buckets[i]);
			}
			free(buckets);
		}
		}
	}
	if (sp->seek_options & RX_SEEK_DISTHIST) { /* This section will create a seek distance histogram */
		buckets = malloc(sp->seek_NumDistHistBuckets * sizeof(seek_t));
		if (buckets == NULL) {
			sprintf(errormessage,"#%s: Cannot allocate %d bytes for distance histogram buckets\n",xgp->progname, sp->seek_NumDistHistBuckets*sizeof(seek_t));
			fprintf(xgp->errout,errormessage);
			fprintf(tmp,errormessage);
		} else {
		for (i = 0; i < sp->seek_NumDistHistBuckets; i++) buckets[i] = 0;
		divisor = longest / sp->seek_NumDistHistBuckets;
		if (divisor == 0) {
			sprintf(errormessage,"#%s: Cannot print Distance histogram - %d is not enough range\n",xgp->progname, sp->seek_NumDistHistBuckets);
			fprintf(xgp->errout,errormessage);
			fprintf(tmp,errormessage);
			free(buckets);  
		} else {
			/* fill the Distance histogram buckets */
			for (i = 1; i < sp->seek_total_ops-1; i++) {
			if (sp->seeks[i].block_location  > sp->seeks[i-1].block_location)
				distance = sp->seeks[i].block_location - sp->seeks[i-1].block_location;
			else distance = sp->seeks[i-1].block_location - sp->seeks[i].block_location;
				bucket = distance/divisor;
				buckets[bucket]++;
			}
			/* print the Distance histgram information for each bucket */
			for (i = 0; i < sp->seek_NumDistHistBuckets; i++) {
#ifdef WIN32
				fprintf(tmp,"#DistHist %04d %10I64u\n",
#else
				fprintf(tmp,"#DistHist %04d %10llu\n",
#endif
					i,buckets[i]);
			}
			free(buckets);
		}
		}
	}
	/* cleanup */
	fclose(tmp);
	if (xgp->global_options & RX_VERBOSE)
		fprintf(xgp->output,"%s: seeks saved in file %s\n",xgp->progname,tmpname);
	return;
} /* end of xdd_save_seek_list()  */
/*----------------------------------------------------------------------------*/
int32_t
xdd_load_seek_list(ptds_t *p) {
	int32_t i;  /* index variable */
	FILE *loadfp; /* Load File Pointer */
	char line[1024]; /* one line of characters */
	char *tp;  /* token pointer */
	int32_t ordinal; /* ordinal number of the seek */
	uint64_t loc;  /* location */
	int32_t reqsz,t1,t2; /* request size, time1 and time2 */
	int32_t reqsz_high; /* highest request size*/
	char rw;  /* read or write operation */
	char *status; /* status of the fgets */
	struct seekhdr *sp;
	sp = &p->seekhdr;
	/* open the load file */
	loadfp = fopen(sp->seek_loadfile,"r");
	if (loadfp == NULL) {
		fprintf(xgp->errout,"%s: Error: Cannot open seek load file %s\n",
			xgp->progname,sp->seek_loadfile);
		return(-1);
	}
	/* read in the load file one line at a time */
	status = line;
	i = 0;
	reqsz_high = 0;
	while (status != NULL) {
		status = fgets(line, sizeof(line), loadfp);
		if (status == NULL ) continue;
		tp = line;
		if (*tp == COMMENT) continue;
		tp = xdd_getnexttoken(tp);
		/* Check for comment line */
		if (*tp == COMMENT) continue;
		/* Check for Directive line */
		if (*tp == DIRECTIVE) {
			xdd_process_directive(tp);
			continue;
		}
		/* Must be a seek line */
#ifdef WIN32
		sscanf(line,"%d %I64u %d %c %d %d",
#else
		sscanf(line,"%d %llu %d %c %d %d",
#endif
			&ordinal,&loc,&reqsz,&rw,&t1,&t2);
		sp->seeks[i].block_location = loc;
		if ((rw == 'w') || (rw == 'W')) 
			sp->seeks[i].operation = RX_WRITE;
		else sp->seeks[i].operation = RX_READ; /* READ */
		sp->seeks[i].reqsize = reqsz;
		sp->seeks[i].time1 = t1;
		sp->seeks[i].time2 = t2;
		if (reqsz > reqsz_high) reqsz_high = reqsz;
		i++;
	}
	sp->seek_iosize = reqsz_high * p->block_size;
	return(0);
} /* end of xdd_load_seek_list() */

