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
 *  I/O Perofrmance, Inc.
 */
/*
   xdd - a basic i/o performance test
*/
#define XDDMAIN /* This is needed to make the xdd_globals pointer a static variable here and an extern everywhere else as defined in xdd.h */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_io_loop() - The actual "inner" loop that does all the I/O operations 
 */
int32_t
xdd_io_loop(ptds_t *p, results_t *results) {
	int32_t  i,j,k;             /* k? random variables */
	int32_t  current_op;        /* current operation number */
	char  error_break;          /* set when it is time to break out */
	int32_t  status;            /* status of I/O operation */
	uint64_t current_position;  /* seek location read from device */
	pclk_t  current_time, start_time, elapsed_time; /* used for implementing the time limits */
    	pclk_t  excess_time;        /* Used by the report_threshold timing */
	int32_t  barrierindx;       /* barrier indx for syncio */
	uint64_t *posp;             /* Position Pointer */
	pclk_t  relative_time;      /* Current time relative to relative_base_time */
	pclk_t  relative_base_time; /* Time relative to 0 from start of pass */
	pclk_t  sleep_time;         /* This is the number of pico seconds to sleep between I/O ops */
	int32_t  sleep_time_dw;     /* This is the amount of time to sleep in milliseconds */
	pclk_t  t1,t2,tt;           /* Used to calculate the timing overhead information */
	pclk_t  time_now;           /* used by the lock step functions */
	ptds_t  *p2;                /* pointer to the ptds of another target */
	int32_t  ping_slave;           /* indicates whether or not to ping the slave */
	int32_t  slave_wait;           /* indicates the slave should wait before continuing on */
	int32_t  slave_loop_counter;
#ifdef WIN32
	uint32_t uj;                /* Random unsigned variable */
	LPVOID lpMsgBuf;
	uint32_t bytesxferred; /* Bytes transferred */
	unsigned long plow;
	unsigned long phi;
#endif
	/* Things used by the read-after-write operations */
#if (IRIX || SOLARIS || HPUX || AIX || ALTIX)
	struct stat64 statbuf;
#elif (LINUX || OSX)
    struct  stat    statbuf;
#endif
	int64_t prev_loc; /* The previous location from a read-after-write message from the writer */
	int64_t prev_len; /* The previous length from a read-after-write message from the writer */
	int64_t data_ready; /* The amount of data taht is ready to be read in a read-after-write */
	int64_t data_length; /* The amount of data that is ready to be read for this operation */

#if (LINUX)
	int	dio;	/* Direct I/O flag used by SG IO */
	int	bs;	/* Block size used by SG IO */
	int	blocks;	/* Number of blocks to transfer used by SG IO */
	int	to_block, from_block;	/* To and From blocks used by SG IO */
#endif


	if (xgp->global_options & RX_TIMER_INFO) {
		pclk_now(&t1);
		for (i=0; i<100; i++) 
			pclk_now(&t2);
		pclk_now(&t2);
		tt=t2-t1;
#ifdef WIN32
		fprintf(stderr,"agv pclk_now=%I64u\n",tt/100);
#else
		fprintf(stderr,"agv pclk_now=%llu\n",tt/100);
#endif
#ifdef WIN32
		for (i=1; i< 1001; i*=10) {
			pclk_now(&t1);
			Sleep(i);
			pclk_now(&tt);
			tt -= t1;
fprintf(stderr,"Requested sleep time in microseconds=%d, Actual sleep time in microseconds=%I64u\n",i*1000,tt/MILLION);
		}
#else
		fprintf(stderr,"cannot run this test...\n");
#endif
	}
	/* Initiialize counters, barriers, clocks, ...etc */
	prev_loc = 0;
	prev_len = 0;
	data_ready = 0;
	data_length = 0;
	barrierindx = 0;
	current_op = 0;
	p->iosize = p->reqsize*p->block_size;
        pclk_now(&start_time);
	relative_base_time = start_time;
	/* Check to see if this thread has a start delay time. If so, just hang out
	 * until the start delay time has elapsed and then spring into action!
	 * The start_delay time is stored as a pclk_t which is units of pico seconds.
	 */
	if (p->start_delay > 0) {
		sleep_time_dw = (int32_t)(p->start_delay/BILLION);
#ifdef WIN32
		Sleep(sleep_time_dw);
#elif (LINUX || IRIX || AIX || ALTIX || OSX) /* Add OS Support to this line for usleep() */
		if ((sleep_time_dw*CLK_TCK) > 1000) /* only sleep if it will be 1 or more ticks */
#if (IRIX || ALTIX)
			sginap((sleep_time_dw*CLK_TCK)/1000);
#elif (LINUX || AIX || OSX) /* Add OS Support to this line for usleep() as well*/
			usleep(sleep_time_dw*1000);
#endif
#endif
	}
	slave_loop_counter = 0;
	if (p->ls_slave >= 0) { /* I am a master */
		p->ls_interval_base_value = 0;
		if (p->ls_interval_type & LS_INTERVAL_TIME) {
			p->ls_interval_base_value = relative_base_time;
		}
		if (p->ls_interval_type & LS_INTERVAL_OP) {
			p->ls_interval_base_value = 0;
		}
		if (p->ls_interval_type & LS_INTERVAL_PERCENT) {
			p->ls_interval_base_value = 1; 
		}
		if (p->ls_interval_type & LS_INTERVAL_BYTES) {
			p->ls_interval_base_value = 0;
		}
	} else { /* I am a slave */
		p->ls_task_base_value = 0;
		if (p->ls_task_type & LS_TASK_TIME) {
			p->ls_task_base_value = relative_base_time;
		}
		if (p->ls_task_type & LS_TASK_OP) {
			p->ls_task_base_value = 0;
		}
		if (p->ls_task_type & LS_TASK_PERCENT) {
			p->ls_task_base_value = 1; 
		}
		if (p->ls_task_type & LS_TASK_BYTES) {
			p->ls_task_base_value = 0;
		}
	}
	/* This is the main loop for a single pass */
	for (i = 0; i < p->total_ops; i++) {
		/* Syncio barrier - wait for all others to get here */
		if ((xgp->syncio > 0) && (xgp->number_of_targets > 1) && (i % xgp->syncio == 0)) {
			xdd_barrier(&xgp->syncio_barrier[barrierindx]);
			barrierindx ^= 1; /* toggle barrier index */
		}
		/* Check to see if we need to wait for another target to trigger us to start.
		 * If so, then enter the trigger_start barrier and have a beer until we get the signal to
		 * jump into action!
		 */
		if ((p->target_options & RX_WAITFORSTART) && (p->run_status == 0)) { 
			/* Enter the barrier and wait to be told to go */
			xdd_barrier(&p->Start_Trigger_Barrier[p->Start_Trigger_Barrier_index]);
			p->Start_Trigger_Barrier_index ^= 1;
			p->run_status = 1; /* indicate that we have been released */
		}
		/* Check to see if we need to signal some other target to start, stop, or pause.
		 * If so, tickle the appropriate semaphore for that target and get on with our business.
		 */
		if (p->trigger_types) {
			p2 = &xgp->ptds[p->start_trigger_target];
			if (p2->run_status == 0) {
				if (p->trigger_types & TRIGGER_STARTTIME) {
					/* If we are past the start time then signal the specified target to start */
					pclk_now(&tt);
					if (tt > (p->start_trigger_time + relative_base_time)) {
						xdd_barrier(&p2->Start_Trigger_Barrier[p2->Start_Trigger_Barrier_index]);
					}
				}
				if (p->trigger_types & TRIGGER_STARTOP) {
					/* If we are past the specified operation, then signal the specified target to start */
					if (i > p->start_trigger_op) {
						xdd_barrier(&p2->Start_Trigger_Barrier[p2->Start_Trigger_Barrier_index]);
					}
				}
				if (p->trigger_types & TRIGGER_STARTPERCENT) {
					/* If we have completed percentage of operations then signal the specified target to start */
					if (i > (p->start_trigger_percent * p->total_ops)) {
						xdd_barrier(&p2->Start_Trigger_Barrier[p2->Start_Trigger_Barrier_index]);
					}
				}
				if (p->trigger_types & TRIGGER_STARTBYTES) {
					/* If we have completed transferring the specified number of bytes, then signal the 
					* specified target to start 
					*/
					if (p->perpass.bytes > p->start_trigger_bytes) {
						xdd_barrier(&p2->Start_Trigger_Barrier[p2->Start_Trigger_Barrier_index]);
					}
				}
			}
		} /* End of the trigger processing */
		/* Check to see if we are in lockstep with another target.
		 * If so, then if we are a master, then do the master thing.
		 * Then check to see if we are also a slave and do the slave thing.
		 */
		if (p->ls_slave >= 0) { /* Looks like we are the MASTER in lockstep with a slave target */
			p2 = &xgp->ptds[p->ls_slave]; /* This is a pointer to the slave ptds */
			ping_slave = FALSE;
			/* Check to see if it is time to ping the slave to do something */
			if (p->ls_interval_type & LS_INTERVAL_TIME) {
				/* If we are past the start time then signal the specified target to start.
				 */
				pclk_now(&time_now);
				if (time_now > (p->ls_interval_value + p->ls_interval_base_value)) {
					ping_slave = TRUE;
					p->ls_interval_base_value = time_now; /* reset the new base time */
				}
			}
			if (p->ls_interval_type & LS_INTERVAL_OP) {
				/* If we are past the specified operation, then signal the specified target to start.
				 */
				if (i >= (p->ls_interval_value + p->ls_interval_base_value)) {
					ping_slave = TRUE;
					p->ls_interval_base_value = i;
				}
			}
			if (p->ls_interval_type & LS_INTERVAL_PERCENT) {
				/* If we have completed percentage of operations then signal the specified target to start.
				 */
				if (i >= ((p->ls_interval_value*p->ls_interval_base_value) * p->total_ops)) {
					ping_slave = TRUE;
					p->ls_interval_base_value++;
				}
			}
			if (p->ls_interval_type & LS_INTERVAL_BYTES) {
				/* If we have completed transferring the specified number of bytes, then signal the 
				 * specified target to start.
				 */
				if (p->perpass.bytes >= (p->ls_interval_value + (int64_t)(p->ls_interval_base_value))) {
					ping_slave = TRUE;
					p->ls_interval_base_value = p->perpass.bytes;
				}
			}
			if (ping_slave) { /* Looks like it is time to ping the slave to do something */
				/* note that the SLAVE owns the mutex and the counter used to tell it to do something */
				/* Get the mutex lock for the ls_task_counter and increment the counter */
				pthread_mutex_lock(&p2->ls_mutex);
				/* At this point the slave could be in one of three places. Thus this next section of code
				 * must be carefully structured to prevent a race condition between the master and the slave.
				 * If the slave is simply waiting for the master to release it, then the ls_task_counter lock
				 * can be released and the master can enter the lock_step_barrier to get the slave going again.
				 * If the slave is running, then it will have to wait for the ls_task_counter lock and
				 * when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
				 * decrement the counter by 1 and continue on executing a lock. 
				 */
				p2->ls_task_counter++;
				/* Now that we have updated the slave's task counter, we need to check to see if the slave is 
				 * waiting for us to give it a kick. If so, enter the barrier which will release the slave and
				 * us as well. 
				 * ****************Currently this is set up simply to do overlapped lockstep.*******************
				 */
				if (p2->ls_ms_state & LS_SLAVE_WAITING) { /* looks like the slave is waiting for something to do */
					p2->ls_ms_state &= ~LS_SLAVE_WAITING;
					pthread_mutex_unlock(&p2->ls_mutex);
					xdd_barrier(&p2->Lock_Step_Barrier[p2->Lock_Step_Barrier_Master_Index]);
					p2->Lock_Step_Barrier_Master_Index ^= 1;
					/* At this point the slave outght to be running. */
				} else {
					pthread_mutex_unlock(&p2->ls_mutex);
				}
			} /* Done pinging the slave */
		} /* end of SLAVE processing as a MASTER */
		if (p->ls_master >= 0) { /* Looks like we are a slave to some other MASTER target */
			/* As a slave, we need to check to see if we should stop running at this point. If not, keep on truckin'.
			 */   
			/* Look at the type of task that we need to perform and the quantity. If we have exhausted
			 * the quantity of operations for this interval then enter the lockstep barrier.
			 */
			pthread_mutex_lock(&p->ls_mutex);
			if (p->ls_task_counter > 0) {
				/* Ahhhh... we must be doing something... */
				slave_wait = FALSE;
				if (p->ls_task_type & LS_TASK_TIME) {
					/* If we are past the start time then signal the specified target to start.
					*/
					pclk_now(&time_now);
					if (time_now > (p->ls_task_value + p->ls_task_base_value)) {
						slave_wait = TRUE;
						p->ls_task_base_value = time_now;
						p->ls_task_counter--;
					}
				}
				if (p->ls_task_type & LS_TASK_OP) {
					/* If we are past the specified operation, then indicate slave to wait.
					*/
					if (i >= (p->ls_task_value + p->ls_task_base_value)) {
						slave_wait = TRUE;
						p->ls_task_base_value = i;
						p->ls_task_counter--;
					}
				}
				if (p->ls_task_type & LS_TASK_PERCENT) {
					/* If we have completed percentage of operations then indicate slave to wait.
					*/
					if (i >= ((p->ls_task_value * p->ls_task_base_value) * p->total_ops)) {
						slave_wait = TRUE;
						p->ls_task_base_value++;
						p->ls_task_counter--;
					}
				}
				if (p->ls_task_type & LS_TASK_BYTES) {
					/* If we have completed transferring the specified number of bytes, then indicate slave to wait.
					*/
					if (p->perpass.bytes >= (p->ls_task_value + (int64_t)(p->ls_task_base_value))) {
						slave_wait = TRUE;
						p->ls_task_base_value = p->perpass.bytes;
						p->ls_task_counter--;
					}
				}
			} else {
				slave_wait = TRUE;
			}
			if (slave_wait) { /* At this point it looks like the slave needs to wait for the master to tell us to do something */
				/* Get the lock on the task counter and check to see if we need to actually wait or just slide on
				 * through to the next task.
				 */
				/* slave_wait will be set for one of two reasons: either the task counter is zero or the slave
				 * has finished the last task and needs to wait for the next ping from the master.
				 */
				/* If the master has finished then don't bother waiting for it to enter this barrier because it never will */
				if ((p->ls_ms_state & LS_MASTER_FINISHED) && (p->ls_ms_state & LS_SLAVE_COMPLETE)) {
					/* if we need to simply finish all this, just release the lock and move on */
					p->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
					if (p->ls_ms_state & LS_MASTER_WAITING) {
						p->ls_ms_state &= ~LS_MASTER_WAITING;
						pthread_mutex_unlock(&p->ls_mutex);
						xdd_barrier(&p->Lock_Step_Barrier[p->Lock_Step_Barrier_Slave_Index]);
						p->Lock_Step_Barrier_Slave_Index ^= 1;
						slave_loop_counter++;
					} else {
						pthread_mutex_unlock(&p->ls_mutex);
					}
				} else if ((p->ls_ms_state & LS_MASTER_FINISHED) && (p->ls_ms_state & LS_SLAVE_STOP)) {
					/* This is the case where the master is finished and we need to stop NOW.
					 * Therefore, release the lock, set the pass_ring to true and break this loop.
					 */
					p->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
					if (p->ls_ms_state & LS_MASTER_WAITING) {
						p->ls_ms_state &= ~LS_MASTER_WAITING;
						pthread_mutex_unlock(&p->ls_mutex);
						xdd_barrier(&p->Lock_Step_Barrier[p->Lock_Step_Barrier_Slave_Index]);
						p->Lock_Step_Barrier_Slave_Index ^= 1;
						slave_loop_counter++;
					} else {
						pthread_mutex_unlock(&p->ls_mutex);
					}
					p->pass_ring = TRUE;
					break;
				} else { /* At this point the master is not finished and we need to wait for something to do */
					if (p->ls_ms_state & LS_MASTER_WAITING)
						p->ls_ms_state &= ~LS_MASTER_WAITING;
					p->ls_ms_state |= LS_SLAVE_WAITING;
					pthread_mutex_unlock(&p->ls_mutex);
					xdd_barrier(&p->Lock_Step_Barrier[p->Lock_Step_Barrier_Slave_Index]);
					p->Lock_Step_Barrier_Slave_Index ^= 1;
					slave_loop_counter++;
				}
			} else {
				pthread_mutex_unlock(&p->ls_mutex);
			}
				/* At this point the slave does not have to wait for anything so keep on truckin... */
		}
		/* init the error number and break flag for good luck */
		errno = 0;
		error_break = 0;
		/* Get the location to seek to */
		if (p->seekhdr.seek_options & RX_SEEK_NONE) /* reseek to starting offset if noseek is set */
				p->my_current_byte_location = (uint64_t)((p->mynum * xgp->target_offset) + p->seekhdr.seeks[0].block_location) * p->block_size;
		else    p->my_current_byte_location = (uint64_t)((p->mynum * xgp->target_offset) + p->seekhdr.seeks[current_op].block_location) * p->block_size;

#if (LINUX || IRIX || SOLARIS || HPUX || AIX || ALTIX || OSX)
		if ((p->target_options & RX_READAFTERWRITE) && (p->target_options & RX_RAW_READER)) { 
// fprintf(stderr,"Reader: RAW check - dataready=%lld, trigger=%x\n",data_ready,p->raw_trigger);
			/* Check to see if we can read more data - if not see where we are at */
			if (p->raw_trigger & RX_RAW_STAT) { /* This section will continually poll the file status waiting for the size to increase so that it can read more data */
				while (data_ready < p->iosize) {
					/* Stat the file so see if there is data to read */
#if (LINUX || OSX)
					status = fstat(p->fd,&statbuf);
#else
					status = fstat64(p->fd,&statbuf);
#endif
					if (status < 0) {
						fprintf(xgp->errout,"%s: RAW: Error getting status on file\n", xgp->progname);
						data_ready = p->iosize;
					} else { /* figure out how much more data we can read */
						data_ready = statbuf.st_size - p->my_current_byte_location;
						if (data_ready < 0) {
							/* The result of this should be positive, otherwise, the target file
							* somehow got smaller and there is a problem. 
							* So, fake it and let this loop exit 
							*/
							fprintf(xgp->errout,"%s: RAW: Something is terribly wrong with the size of the target file...\n",xgp->progname);
							data_ready = p->iosize;
						}
					}
				}
			} else { /* This section uses a socket connection to the writer and waits for the writer to tell it to read something */
//fprintf(stderr,"data_ready=%lld, current_op=%d,prev_loc=%lld, prev_len=%lld\n",data_ready, current_op,prev_loc, prev_len);
				while (data_ready < p->iosize) {
					/* xdd_raw_read_wait() will block until there is data to read */
					status = xdd_raw_read_wait(p);
					if (p->raw_msg.length != p->iosize) 
#ifdef WIN32
						fprintf(stderr,"error on msg recvd %d loc %I64d, length %I64d\n",p->raw_msg_recv-1, p->raw_msg.location,  p->raw_msg.length);
#else
						fprintf(stderr,"error on msg recvd %d loc %lld, length %lld\n",p->raw_msg_recv-1, p->raw_msg.location,  p->raw_msg.length);
#endif
					if (p->raw_msg.sequence != p->raw_msg_sequence) {
#ifdef WIN32
						fprintf(stderr,"sequence error on msg recvd %d loc %I64d, length %I64d seq num is %d should be %d\n",
#else
						fprintf(stderr,"sequence error on msg recvd %d loc %lld, length %lld seq num is %d should be %d\n",
#endif
							p->raw_msg_recv-1, p->raw_msg.location,  p->raw_msg.length, p->raw_msg.sequence, p->raw_msg_sequence);
					}
					if (p->raw_msg_sequence == 0) { /* this is the first message so prime the prev_loc and length with the current values */
						prev_loc = p->raw_msg.location;
						prev_len = 0;
					} else if (p->raw_msg.location <= prev_loc) 
						/* this message is old and can be discgarded */
						continue;
					p->raw_msg_sequence++;
					/* calculate the amount of data to be read between the end of the last location and the end of the current one */
					data_length = ((p->raw_msg.location + p->raw_msg.length) - (prev_loc + prev_len));
					data_ready += data_length;
					if (data_length > p->iosize) 
						fprintf(stderr,"msgseq=%d, loc=%lld, len=%lld, data_length is %lld, data_ready is now %lld, iosize=%d\n",p->raw_msg.sequence, p->raw_msg.location, p->raw_msg.length, data_length, data_ready, p->iosize );
					prev_loc = p->raw_msg.location;
					prev_len = data_length;
				}
			}
		} /* End of dealing with a read-after-write */
#endif
		/* get the size of this I/O operation and convert it to bytes */
		p->iosize = p->seekhdr.seeks[current_op].reqsize * p->block_size;
		/* If time stamping is on then record some information */
		if (p->ts_options & TS_ON) { /* We record information only if the trigger time or operation has been reached or if we record all */
			pclk_now(&tt);
			if ((p->ts_options & TS_TRIGGERED) || 
				(p->ts_options & TS_ALL) ||
				((p->ts_options & TS_TRIGTIME) && (tt >= p->ts_trigtime)) ||
				((p->ts_options & TS_TRIGOP) && (p->ts_trigop == i))) {
				p->ts_options |= TS_TRIGGERED;
				p->ttp->tte[p->ttp->tte_indx].rwvop = p->seekhdr.seeks[current_op].operation;
				p->ttp->tte[p->ttp->tte_indx].pass = p->current_pass;
				p->ttp->tte[p->ttp->tte_indx].byte_location = p->my_current_byte_location;
				p->ttp->tte[p->ttp->tte_indx].opnumber = current_op;
				p->ttp->tte[p->ttp->tte_indx].start = tt;
				p->timestamps++;
			}
		}
		/* If this is a 'throttled' operation, check to see what time it is relative to the start
		 * of this pass, compare that to the time that this operation was supposed to begin, and
		 * go to sleep for how ever many milliseconds is necessary until the next I/O needs to be
		 * issued. If we are past the issue time for this operation, just issue the operation.
		 */
		if (p->throttle > 0.0) {
			pclk_now(&relative_time);
			relative_time -= relative_base_time;
			if (relative_time < p->seekhdr.seeks[current_op].time1) { /* Then we may need to sleep */
				sleep_time = (p->seekhdr.seeks[current_op].time1 - relative_time) / BILLION; /* sleep time in milliseconds */
				if (sleep_time > 0) {
					sleep_time_dw = sleep_time;
#ifdef WIN32
					Sleep(sleep_time_dw);
#elif (LINUX || IRIX || AIX || ALTIX || OSX) /* Change this line to use usleep */
					if ((sleep_time_dw*CLK_TCK) > 1000) /* only sleep if it will be 1 or more ticks */
#if (IRIX || ALTIX)
						sginap((sleep_time_dw*CLK_TCK)/1000);
#elif (LINUX || AIX || OSX) /* Change this line to use usleep */
						usleep(sleep_time_dw*1000);
#endif
#endif
				}
			}
		}
		/* Check the data transfer size at this point. If this is the last I/O operation it is possible
		 * that the data left to transfer is smaller than the iosize. Therefore, make the last I/O small
		 * so that we don't overrun the file/device boundary.
		 */
//fprintf(stderr,"op=%d, bytestoxfer=%lld, perpass.bytes=%lld\n",i, p->bytestoxfer, p->perpass.bytes);
		if ((p->bytestoxfer - p->perpass.bytes) < p->iosize)
			p->actual_iosize = (p->bytestoxfer - p->perpass.bytes);
		else p->actual_iosize = p->iosize;
//fprintf(stderr, "iosize=%d, actual iosize=%d\n",p->iosize, p->actual_iosize);
		
#ifdef WIN32
		plow = (unsigned long)p->my_current_byte_location;
		phi = (unsigned long)(p->my_current_byte_location >> 32);

		/* Position to the correct place on the storage device/file */
		SetFilePointer(p->fd, plow, &phi, FILE_BEGIN);
		/* Check to see if there is supposed to be a sequenced data pattern in the data buffer */
		/* For write operations, this means that we should update the data pattern in the buffer to
		 * the expected value which is the relative byte offset of each block in this request.
		 * For read operations, we need to check the block offset with what we think it should be.
		 * For read operations, it is assumed that the data read was previously written by xdd
		 * and is in the expected format.
		 */  
		/* If the SEQUENCED PATTERN option was specified, then this will fill the write buffer with
		 * the appropriate pattern before the write operation is started.
		 */
        pclk_now(&p->my_current_start_time);
		if (i == 0) /* record our starting time */
			p->my_start_time = p->my_current_start_time;
		if (p->seekhdr.seeks[current_op].operation == RX_WRITE) {
			if (p->target_options & RX_SEQUENCED_PATTERN) {
				posp = (uint64_t *)p->rwbuf;
				for (uj=0; uj<(p->actual_iosize/sizeof(p->my_current_byte_location)); uj++) {
					*posp = p->my_current_byte_location + (uj * sizeof(p->my_current_byte_location));
					posp++;
				}
			}
			/* Actually write the data to the storage device/file */
			status = WriteFile(p->fd, p->rwbuf, p->actual_iosize, &bytesxferred, NULL);
		} else { /* Simply do the normal read operation */
			status = ReadFile(p->fd, p->rwbuf, p->actual_iosize, &bytesxferred, NULL);
			if (p->target_options & (RX_VERIFY_CONTENTS | RX_VERIFY_LOCATION)) {
				posp = (uint64_t *)p->rwbuf;
				current_position = *posp; 
			}
		}
        pclk_now(&p->my_current_end_time);
		/* Take a time stamp if necessary */
		if ((p->ts_options & TS_ON) && (p->ts_options & TS_TRIGGERED)) {  
			p->ttp->tte[p->ttp->tte_indx++].end = p->my_current_end_time;
			if (p->ttp->tte_indx == p->ttp->tt_size) { /* Check to see if we are at the end of the buffer */
				if (p->ts_options & TS_ONESHOT) 
					p->ts_options &= ~TS_ON; /* Turn off Time Stamping now that we are at the end of the time stamp buffer */
				else if (p->ts_options & TS_WRAP) 
					p->ttp->tte_indx = 0; /* Wrap to the beginning of the time stamp buffer */
			}
		}
		/* Let's check the status of the last operation to see if things went well.
		 * If not, tell somebody who cares - like the poor soul running this program.
		 */
		if ((status == FALSE) || (bytesxferred != (unsigned long)p->actual_iosize)) { 
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
			fprintf(xgp->errout,"%s: I/O error: could not %s target %s\n",
				xgp->progname,(p->seekhdr.seeks[current_op].operation == RX_WRITE)?"write":"read",p->target);
			fprintf(xgp->errout,"reason:%s",lpMsgBuf);
			fflush(xgp->errout);
			results->errors++;
		}
		status = bytesxferred;
#else /* UUUUUUUUUUUUUUUU Begin Unix stuff UUUUUUUUUUUUUUUUU*/
#if (IRIX || SOLARIS || HPUX || AIX || ALTIX)
		lseek64(p->fd,(off64_t)p->my_current_byte_location,0); 
#elif (LINUX || OSX)
		/* In Linux the -D_FILE_OFFSET_BITS=64 make the off_t type be a 64-bit integer */
                if (!(p->target_options & RX_SGIO)) 
		    lseek(p->fd, (off_t)p->my_current_byte_location, SEEK_SET);
#endif
        /* Record the starting time for this operation */
        pclk_now(&p->my_current_start_time);
		if (i == 0) /* record our starting time */
			p->my_start_time = p->my_current_start_time;

		/* Do the deed .... */
		if (p->seekhdr.seeks[current_op].operation == RX_WRITE) {  
			/* Sequenced Data Pattern */
			if (p->target_options & RX_SEQUENCED_PATTERN) {
				posp = (uint64_t *)p->rwbuf;
				for (j=0; j<(p->actual_iosize/sizeof(p->my_current_byte_location)); j++) {
					*posp = p->my_current_byte_location + (j * sizeof(p->my_current_byte_location));
					posp++;
				}
			}
#if (HPSS)
			if (p->target_options & RX_HPSS) 
				 status = hpss_Write(p->fd, p->rwbuf, p->actual_iosize);
			else status = write(p->fd, p->rwbuf, p->actual_iosize);
#else
			if ((p->target_options & RX_SGIO)) {
#if (LINUX)
                                 dio = (p->target_options & RX_DIO);
                                 bs = 512; // This is because sg uses a sector size block size
                                 blocks = p->actual_iosize / bs;
                                 to_block = (p->my_current_byte_location / bs);
				 status = xdd_sg_write(p->fd, p->rwbuf, blocks, to_block, bs, &dio);
#endif
			} else status = write(p->fd, p->rwbuf, p->actual_iosize);
#endif
		} else { /* Simply do the normal read operation */
#if (HPSS)
			if (p->target_options & RX_HPSS) 
				 status = hpss_Read(p->fd, p->rwbuf, p->actual_iosize);
			else status = read(p->fd, p->rwbuf, p->actual_iosize);
#else
			if ((p->target_options & RX_SGIO)) {
#if (LINUX)
                                 dio = (p->target_options & RX_DIO);
                                 bs = 512; // This is because sg uses a sector size block size
                                 blocks = p->actual_iosize / bs;
                                 from_block = (p->my_current_byte_location / bs);
				 status = xdd_sg_read(p->fd, p->rwbuf, blocks, from_block, bs, &dio);
#endif
                        } else status = read(p->fd, p->rwbuf, p->actual_iosize);
#endif
			if (p->target_options & (RX_VERIFY_CONTENTS | RX_VERIFY_LOCATION)) {
				posp = (uint64_t *)p->rwbuf;
				current_position = *posp; 
			}
		}
        /* Record the ending time for this operation */
        pclk_now(&p->my_current_end_time);
		/* Time stamp! */
		if ((p->ts_options & TS_ON) && (p->ts_options & TS_TRIGGERED)) {
			p->ttp->tte[p->ttp->tte_indx++].end = p->my_current_end_time;
			if (p->ts_options & TS_ONESHOT) { /* Check to see if we are at the end of the buffer */
				if (p->ttp->tte_indx == p->ttp->tt_size)
					p->ts_options &= ~TS_ON; /* Turn off Time Stamping now that we are at the end of the time stamp buffer */
			} else if (p->ts_options & TS_WRAP) 
					p->ttp->tte_indx = 0; /* Wrap to the beginning of the time stamp buffer */
		}
		/* Check status of the last operation */
		if ((status < 0) || (status != p->actual_iosize)) {
			fprintf(xgp->errout, "(%d.%d) %s: I/O error on target %s - status %d, iosize %d, op %d\n",
				p->mynum,p->myqnum,xgp->progname,p->target,status,p->actual_iosize,i);
			fflush(xgp->errout);
			perror("reason");
			results->errors++;
		}
#endif /* UUUUUUUUUUUUUUU for UNIX stuff UUUUUUUUUUUUUUU*/
        p->my_current_elapsed_time = p->my_current_end_time - p->my_current_start_time;
        if (p->report_threshold) {
            if (p->my_current_elapsed_time > p->report_threshold) {
                excess_time = (p->my_current_elapsed_time - p->report_threshold)/MILLION;
#ifdef WIN32
                fprintf(xgp->output, "%s: Target %d <%s> Threshold, %I64d, exceeded by, %I64d, microseconds, IO time was, %I64d, usec on block, %I64d\n",
#else
                fprintf(xgp->output, "%s: Target %d <%s> Threshold, %lld, exceeded by, %lld, microseconds, IO time was, %lld, usec on block, %lld\n",
#endif
                    xgp->progname, p->mynum, p->target, p->report_threshold/MILLION, excess_time, p->my_current_elapsed_time/MILLION, p->my_current_byte_location);
            }
        }
        
#if (LINUX || IRIX || SOLARIS || HPUX || AIX || ALTIX || OSX)
		if ((p->target_options & RX_READAFTERWRITE) && (p->target_options & RX_RAW_WRITER)) {
			/* Since I am the writer in a read-after-write operation, and if we are using a socket connection to the reader for write-completion messages
			 * then I need to send the reader a message of what I just wrote - starting location and length of write.
			 */
		}
#endif

		if ((p->seekhdr.seeks[current_op].operation == RX_READ) && 
			(p->target_options & (RX_VERIFY_CONTENTS | RX_VERIFY_LOCATION)))
			p->perpass.compare_errors += xdd_verify(p, current_op);

		/* Get the current time stamp */
		pclk_now(&current_time);
		/* Check for errors in the last operation */
		if (results->errors >= xgp->max_errors) {
			fprintf(xgp->errout, "(%d) %s: Maximum error threshold reached on target %s - exiting\n",
				p->mynum,xgp->progname,p->target);
			fflush(xgp->errout);
			error_break = 1;
		}
		if ((status == 0) && (errno == 0)) {
			fprintf(xgp->errout, "(%d) %s: End-Of-File reached on target %s\n",
				p->mynum,xgp->progname,p->target);
			fflush(xgp->errout);
			error_break = 1; /* fake an exit */
		}
// fprintf(stderr,"write status = %d\n",status);
		/* Only add to the total number of bytes  & ops if data was actually transferred */
		if (status > 0) {
			results->bytes += status;
			results->ops++;
			results->accum_time = (current_time - start_time);
			if (p->target_options & RX_READAFTERWRITE){
				if (p->target_options & RX_RAW_READER) { 
					data_ready -= status;
				} else { /* I must be the writer, send a message to the reader if requested */
					if (p->raw_trigger & RX_RAW_MP) {
						p->raw_msg.magic = RX_RAW_MAGIC;
						p->raw_msg.length = status;
						p->raw_msg.location = p->my_current_byte_location;
						xdd_raw_writer_send_msg(p);
					}
				}
			}
		}
		if (p->time_limit) {
			elapsed_time = current_time - start_time;
			if (elapsed_time >= (pclk_t)(p->time_limit*TRILLION))
				p->pass_ring = 1;
		}
		if (xgp->estimated_end_time > 0) {
			if (current_time >= xgp->estimated_end_time)
				xgp->run_ring = 1;
		}
		if (p->pass_ring || xgp->run_ring || error_break || xgp->deskew_ring) break;
		current_op++;
	} /* end of FOR loop where i=total_ops */
	/* If the deskew option was selected then set the deskew ring so that all the other threads stop now. */
	if (xgp->global_options & RX_DESKEW)
		xgp->deskew_ring = 1;
	/* check to see if the writes ought to be flushed and flush them */
	if (p->target_options & RX_SYNCWRITE) {
#ifdef WIN32
	status = FlushFileBuffers(p->fd);
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
		fprintf(xgp->errout,"%s: I/O error: could not flush buffers to target %s\n",
			xgp->progname,p->target);
		fprintf(xgp->errout,"reason:%s",lpMsgBuf);
		fflush(xgp->errout);
	}
#else
	if (p->rwratio < 1.0) {
		status = fsync(p->fd);
		if (status) {
			fprintf(xgp->errout, "(%d) %s: sync error on target %s\n",
				p->mynum,xgp->progname,p->target);
			fflush(xgp->errout);
			perror("reason");
		}
	}
#endif
	}
	/* This ought to return something other than 0 but this is it for now... */
	/* If there is a slave to this target then we need to tell the slave that we (the master) are finished
	 * and that it ought to abort or finish (depending on the command-line option) but in either case it
	 * should no longer wait for the master to tell it to do something.
	 */
	if (p->ls_master >= 0) {
		/* If this is the slave and we are finishing first, turn off the SLAVE_WAITING flag so the 
		 * master does not inadvertently wait for the slave to complete.
		 */
		pthread_mutex_lock(&p->ls_mutex);
		p->ls_ms_state &= ~LS_SLAVE_WAITING;
		p->ls_ms_state |= LS_SLAVE_FINISHED;
		pthread_mutex_unlock(&p->ls_mutex);
	}
	if (p->ls_slave >= 0) {  
		/* Get the mutex lock for the ls_task_counter and increment the counter */
		p2 = &xgp->ptds[p->ls_slave];
		pthread_mutex_lock(&p2->ls_mutex);
		p2->ls_ms_state |= LS_MASTER_FINISHED;
		/* At this point the slave could be in one of three places. Thus this next section of code
			* must be carefully structured to prevent a race condition between the master and the slave.
			* If the slave is simply waiting for the master to release it, then the ls_task_counter lock
			* can be released and the master can enter the lock_step_barrier to get the slave going again.
			* If the slave is running, then it will have to wait for the ls_task_counter lock and
			* when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
			* decrement the counter by 1 and continue on executing a lock. 
			*/
		p2->ls_task_counter++;
		if (p2->ls_ms_state & LS_SLAVE_WAITING) { /* looks like the slave is waiting for something to do */
			p2->ls_ms_state &= ~LS_SLAVE_WAITING;
			pthread_mutex_unlock(&p2->ls_mutex);
			xdd_barrier(&p2->Lock_Step_Barrier[p2->Lock_Step_Barrier_Master_Index]);
			p2->Lock_Step_Barrier_Master_Index ^= 1;
		} else {
			pthread_mutex_unlock(&p2->ls_mutex);
		}
	}
	return(0);
} /* end of xdd_io_loop() */
/*----------------------------------------------------------------------------*/
/* xdd_io_thread() - control of the I/O for the specified reqsize      
 * Notes: Two thread_barriers are used due to an observed race-condition
 * in the xdd_barrier() code, most likely in the semaphore op,
 * that causes some threads to be woken up prematurely and getting the
 * intended synchronization out of sync hanging subsequent threads.
 */
void *
xdd_io_thread(void *pin) {
	int32_t  i,j;  /* k? random variables */
	int32_t  barrierindx; /* Thread barrier index */
	int32_t  QThreadBarrier_Index;
	clock_t  previous_time;  /* previous elpased wall clock time */
	struct tms previous_cpu_times; /* previous CPU user and system times */
	clock_t  current_time; /* Current elpased wall clock time */
	struct tms current_cpu_times; /* Current CPU user and system times */
	pclk_t  CurrentLocalTime; /* The time */
	pclk_t  TimeDelta; /* The difference between now and the Actual Local Start Time */
	uint32_t sleepseconds; /* Number of seconds to sleep while waiting for the global time to start */
	results_t final_results; /* Results structure that contains the *combined* results from all targets, all passes */
	int32_t  status;
	ptds_t  *qp;   /* Pointer to a qthread ptds */
	ptds_t  *tp;   /* Pointer to a target ptds */
	char errmsg[256];
#ifdef WIN32
	LPVOID lpMsgBuf; /* Used for the error messages */
	HANDLE tmphandle;
#endif


	ptds_t *p = (ptds_t *)pin; 
	QThreadBarrier_Index = 0;
#if (AIX)
		p->mythreadid = thread_self();
#elif (LINUX)
		p->mythreadid = pthread_self();
#else
		p->mythreadid = p->mypid;
#endif
#ifdef WIN32
	/* This next section is used to open the Mutex locks that are used by the semaphores.
	 * This is a Windows-only thing to circumvent a bug in their Mutex inheritence
	 */
	for (j=0; j<2; j++) {
		tmphandle = OpenMutex(SYNCHRONIZE,TRUE,xgp->QThread_Barrier[p->mynum][j].mutex.name);
		if (tmphandle == NULL) {
			fprintf(stderr,"Cannot open mutex for Target %d, qthread %d barrier %d\n",p->mynum,p->myqnum,j);
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
		} else xgp->QThread_Barrier[p->mynum][j].mutex.mutex = tmphandle;
	}
	/* If TS is on, then init the ts mutex for later */
	if (p->ts_options & TS_ON) {
		tmphandle = OpenMutex(SYNCHRONIZE,TRUE,xgp->ts_serializer_mutex_name);
			if (tmphandle == NULL) {
				fprintf(stderr,"Cannot open ts serializer mutex for Target %d, qthread %d\n",
					p->mynum,p->myqnum);
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
			} else p->ts_serializer_mutex = tmphandle;
	}
#endif // Section that deals with MUTEX inheritence 

	/* Now lets get down to business... */
	p->iosize = p->reqsize*p->block_size;
	if (p->iosize == 0) {
		fprintf(xgp->errout,"%s: ALERT! iothread for target %d queue %d has an iosize of 0, reqsize of %d, blocksize of %d\n",
			xgp->progname, p->mynum, p->myqnum, p->reqsize, p->block_size);
		fflush(xgp->errout);
	}
	if (p->numreqs != 0) 
		p->bytestoxfer = (uint64_t)(p->numreqs * p->iosize);
	else if (p->mbytes > 0)
		p->bytestoxfer = (uint64_t)p->mbytes * ONEMEG;
	else if (p->kbytes > 0)
		p->bytestoxfer = (uint64_t)p->kbytes * 1024;
	/* This calculates the number of iosize (or smaller) operations that need to be performed. 
	 * In the event the number of bytes to transfer is not an integer number of iosize requests then 
	 * the total number of ops is incremented by 1 and the last I/O op will be the residual.
	 */
	p->total_ops = p->bytestoxfer / p->iosize;
	if (p->bytestoxfer % p->iosize) {
		p->total_ops++;
		p->last_iosize = p->bytestoxfer % p->iosize;
	}
	/* Assign me to the proper processor if I have a processor number 0 or greater. */
	if (p->processor != -1)
		xdd_processor(p);

	/* open the target device */
	p->fd = xdd_open_target(p);
	if ((unsigned int)p->fd == -1) { /* error openning target */
		fprintf(xgp->errout,"%s: Aborting I/O for target %d due to open failure\n",xgp->progname,p->mynum);
		fflush(xgp->errout);
		xgp->abort_io = 1;
		/* Enter the serializer barrier so that the next thread can start */
		xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
		return(0);
	}

	/* set up the seek list */
	p->seekhdr.seek_total_ops = p->total_ops;
	p->seekhdr.seeks = (seek_t *)calloc((int32_t)p->seekhdr.seek_total_ops,sizeof(seek_t));
	if (p->seekhdr.seeks == 0) {
		fprintf(xgp->errout,"%s: cannot allocate memory for seek list\n",xgp->progname);
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
		return(0);
	}
	xdd_init_seek_list(p);
	/* allocate a buffer and re-align it if necessary */
	p->rwbuf = xdd_init_io_buffers(p);
	if (p->rwbuf == NULL) { /* error allocating memory for I/O buffer */
		fprintf(xgp->errout,"%s: Aborting I/O for target %d due to I/O memory buffer allocation failure\n",xgp->progname,p->mynum);
		fflush(xgp->errout);
		xgp->abort_io = 1;
		/* Enter the serializer barrier so that the next thread can start */
		xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
		return(0);
	}
	p->rwbuf_save = p->rwbuf; 
    xdd_pattern_buffer(p); // Put the correct data pattern in the buffer

	if (p->align != getpagesize()) 
			p->rwbuf +=  p->align;
	/* set up the timestamp table */
	xdd_ts_setup(p);
	/* set up for the big loop */
	if (xgp->max_errors == 0) 
		xgp->max_errors = p->total_ops;
	xdd_init_results(&p->qthread_results);
	if (p->rwratio == 1.0) 
		strcpy(p->qthread_results.optype, "read");
	else if (p->rwratio == 0.0)
		strcpy(p->qthread_results.optype, "write");
	else strcpy(p->qthread_results.optype, "mixed");

	xdd_init_results(&p->target_results);	
	if (p->rwratio == 1.0) 
		strcpy(p->target_results.optype, "read");
	else if (p->rwratio == 0.0)
		strcpy(p->target_results.optype, "write");
	else strcpy(p->target_results.optype, "mixed");

	/* If we are synchronizing to a Global Clock, let's synchronize
	 * here so that we all start at *roughly* the same time
	 */
	if (xgp->gts_addr) {
		pclk_now(&CurrentLocalTime);
		while (CurrentLocalTime < xgp->ActualLocalStartTime) {
		    TimeDelta = ((xgp->ActualLocalStartTime - CurrentLocalTime)/TRILLION);
	    	    if (TimeDelta > 2) {
			sleepseconds = TimeDelta - 2;
			sleep(sleepseconds);
		    }
		    pclk_now(&CurrentLocalTime);
		}
	}
	if (xgp->global_options & RX_TIMER_INFO) {
		fprintf(xgp->errout,"Starting now...\n");
		fflush(xgp->errout);
	}
	/* Get the current CPU user and system times */
	previous_time = times(&previous_cpu_times);
	/* This section will initialize the slave side of the lock step mutex and barriers */
	if (p->ls_master >= 0) { /* This means that this target has a master and therefore must be a slave */
		/* init the task counter mutex and the lock step barrier */
		status = pthread_mutex_init(&p->ls_mutex, 0);
		if (status) {
			sprintf(errmsg,"Error initializing lock step slave target %d task counter mutex",p->mynum);
			perror(errmsg);
			fprintf(xgp->errout,"%s: Aborting I/O for target %d due to lockstep mutex allocation failure\n",xgp->progname,p->mynum);
			fflush(xgp->errout);
			xgp->abort_io = 1;
			/* Enter the serializer barrier so that the next thread can start */
			xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
			return(0);
		}
		for (i=0; i<=1; i++) {
				sprintf(p->Lock_Step_Barrier[i].name,"LockStep_T%d_%d",p->ls_master,p->mynum);
				xdd_init_barrier(&p->Lock_Step_Barrier[i], 2, p->Lock_Step_Barrier[i].name);
		}
		p->Lock_Step_Barrier_Slave_Index = 0;
		p->Lock_Step_Barrier_Master_Index = 0;
	} else { /* Make sure these are uninitialized */
		for (i=0; i<=1; i++) {
				p->Lock_Step_Barrier[i].initialized  = FALSE;
		}
		p->Lock_Step_Barrier_Slave_Index = 0;
		p->Lock_Step_Barrier_Master_Index = 0;
	}
	/* Check to see if we will be waiting for a start trigger. 
	 * If so, then init the start trigger barrier.
	 */
	if (p->target_options & RX_WAITFORSTART) {
		for (i=0; i<=1; i++) {
				sprintf(p->Start_Trigger_Barrier[i].name,"StartTrigger_T%d_%d",p->mynum,p->myqnum);
				xdd_init_barrier(&p->Start_Trigger_Barrier[i], 2, p->Start_Trigger_Barrier[i].name);
		}
		p->Start_Trigger_Barrier_index = 0;
		p->run_status = 0;
	}
	/* This section will check to see if we are doing a read-after-write and initialize as needed.
	 * If we are a reader then we need to init the socket and listen for the writer.
	 * If we are the writer, then we assume that the reader has already been started on another
	 * machine and all we need to do is connect to that reader and say Hello.
	 */
	if (p->target_options & RX_READAFTERWRITE) {
		if (p->rwratio == 1.0 )
			xdd_raw_reader_init(p);
		else xdd_raw_writer_init(p);
	}
	/* If I am the last thread to start (mynum == number_of_targets-1)
	 * then set the estimated end time if a -runtime was specified.
	 */
	if ((p->mynum == (xgp->number_of_targets-1)) && (p->myqnum == (p->queue_depth-1))) {
		xgp->run_ring = 0; /* This is the alarm that goes off when the run time is up */
		if (xgp->runtime > 0) {
			pclk_now(&xgp->estimated_end_time);
			xgp->estimated_end_time += ((int64_t)(xgp->runtime*TRILLION)); 
                }
		else xgp->estimated_end_time = 0;
	}
    
    /* Display all the information related to this target */
    xdd_target_info(xgp->output, p);
	if (xgp->csvoutput) 
		xdd_target_info(xgp->csvoutput, p);

	/* Enter the serializer barrier so that the next thread can start */
	xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
	barrierindx = 0;
	/* Start the main pass loop */
	for (p->current_pass = 1; p->current_pass <= xgp->passes; p->current_pass++) {
		xdd_init_results(&p->perpass);
		if (((p->target_options & RX_CREATE_NEW_FILES) || (p->target_options & RX_REOPEN)) && (p->myqnum == 0)){
			p->perpass.open_start_time = p->open_start_time;
			p->perpass.open_end_time = p->open_end_time;
		}
		if (p->rwratio == 1.0) 
			strcpy(p->perpass.optype, "read");
		else if (p->rwratio == 0.0)
			strcpy(p->perpass.optype, "write");
		else strcpy(p->perpass.optype, "mixed");

		/* Before we get started, check to see if we need to reset the 
		 * run_status in case we are using the start trigger.
		 */
		if (p->target_options & RX_WAITFORSTART) 
			p->run_status = 0;
		/* place barrier to ensure that everyone starts at same time */
		if ((!(xgp->global_options & RX_NOBARRIER)) && (xgp->number_of_iothreads > 1)) {
			xdd_barrier(&xgp->thread_barrier[barrierindx]);
			barrierindx ^= 1; /* toggle the barrier index */
		}
		/* Check to see if any of the other threads have aborted */
		if (xgp->abort_io) {
			fprintf(xgp->errout,"%s: Aborting I/O for target %d\n",xgp->progname,p->mynum);
			fflush(xgp->errout);
			break;
		}
		/* set up the alarm clock if there is a time limit on this */
		p->pass_ring = 0;
		/* Get the starting time stamp */
		if (p->current_pass == 1) {
			pclk_now(&p->qthread_results.earliest_start_time);
			p->qthread_results.start_time = p->qthread_results.earliest_start_time;
			p->perpass.start_time = p->qthread_results.earliest_start_time;
		} else pclk_now(&p->perpass.start_time);
		/* Do the actual I/O Loop */
		xdd_io_loop(p, &p->perpass);
		/* Get the ending time stamp */
		pclk_now(&p->perpass.end_time);
#ifdef ndef
		if (p->target_options & RX_RAW_WRITER) { /* Wait for final response from reader */
fprintf(stderr,"I am the raw writer - waiting for response from reader\n");
			status = recv(p->raw_sd, &p->raw_msg, sizeof(p->raw_msg), MSG_WAITALL);
			if (status != sizeof(p->raw_msg)) {
				fprintf(xgp->errout, "%s: bad response from reader: status=%d\n", xgp->progname, status);
			} else fprintf(stderr,"response from reader: raw_msg loc=%lld, len=%lld\n",p->raw_msg.location, p->raw_msg.length);
		} else {
			if (p->target_options & RX_RAW_READER) { /* send a response to the writer that we are finished */
fprintf(stderr,"I am the raw reader - sending final response to writer - %d bytes\n", sizeof(p->raw_msg));
				p->raw_msg.location = p->raw_msg.length = 1;
				status = send(p->raw_sd, &p->raw_msg, sizeof(p->raw_msg), 0);
				if (status != sizeof(p->raw_msg)) {
					fprintf(stderr,"%s: error sending final response to writer - status=%d\n",xgp->progname,status);
					perror("reason");
				}
			}
		}
#endif
		/* Wait for all the other qthreads to complete before we continue */
		xdd_barrier(&xgp->QThread_Barrier[p->mynum][QThreadBarrier_Index]);
		QThreadBarrier_Index ^= 1;
		/* Get the current CPU user and system times */
		current_time = times(&current_cpu_times);
		/* increment the byte and op counters */
		p->qthread_results.bytes += p->perpass.bytes;
		p->qthread_results.ops += p->perpass.ops;
		p->qthread_results.compare_errors += p->perpass.compare_errors;
		/* calculate the elapsed time for this last pass */
		p->perpass.elapsed = (p->perpass.end_time - p->perpass.start_time)/(1000000000000.0);
		p->qthread_results.elapsed += p->perpass.elapsed;
		/* Calculate rates - remember: all times are in seconds */
		if (p->iosize == 0) {
			fprintf(xgp->errout,"%s: ALERT! iothread for target %d queue %d has a perpass elapsed time of 0 on pass %d, setting elapsed to -1\n",
				xgp->progname, p->mynum, p->myqnum, p->current_pass);
			fflush(xgp->errout);
			p->perpass.elapsed = -1;
		}
		p->perpass.avg_rate = (p->perpass.bytes / p->perpass.elapsed) / 1000000.0;
		p->perpass.avg_iops = (p->perpass.ops / p->perpass.elapsed);
		p->perpass.avg_latency = 1.0 / p->perpass.avg_iops;
		/* update the cumulative results for this thread */
		if (p->perpass.avg_rate > p->qthread_results.hi_rate) {
			p->qthread_results.hi_rate = p->perpass.avg_rate;
		}
		if (p->perpass.avg_rate < p->qthread_results.low_rate) {
			p->qthread_results.low_rate = p->perpass.avg_rate;
		}
		if (p->perpass.elapsed > p->qthread_results.hi_elapsed) {
			p->qthread_results.hi_elapsed = p->perpass.elapsed;
		}
		if (p->perpass.elapsed < p->qthread_results.low_elapsed) {
			p->qthread_results.low_elapsed = p->perpass.elapsed;
		}
		/* Calculate the user and system times and percentages used */
		p->perpass.user_time = (current_cpu_times.tms_utime - previous_cpu_times.tms_utime)/((double) CLK_TCK);
		p->perpass.system_time = (current_cpu_times.tms_stime - previous_cpu_times.tms_stime) / ((double) CLK_TCK);
		p->perpass.us_time = p->perpass.user_time + p->perpass.system_time;
		p->perpass.wall_clock_time = (current_time - previous_time) / ((double)CLK_TCK);
		if (p->perpass.wall_clock_time != 0) /* sometimes the wallclock time is 0 for very fast runs */
			p->perpass.percent_cpu = (p->perpass.us_time / p->perpass.elapsed) * 100.0;
		else p->perpass.percent_cpu = 0.0;
		memcpy((unsigned char *)&previous_cpu_times, (unsigned char *)&current_cpu_times, sizeof(struct tms));
		previous_time = current_time;
		p->qthread_results.wall_clock_time += p->perpass.wall_clock_time;
		p->qthread_results.user_time += p->perpass.user_time;
		p->qthread_results.system_time += p->perpass.system_time;
		p->qthread_results.percent_cpu = ((p->qthread_results.user_time + p->qthread_results.system_time)/p->qthread_results.elapsed) * 100.0;
		p->qthread_results.open_time += (p->perpass.open_end_time - p->perpass.open_start_time);
		p->qthread_results.passes_completed++;
		/* Wait at this barrier to make sure all the qthreads have calculated their results */
		xdd_barrier(&xgp->QThread_Barrier[p->mynum][QThreadBarrier_Index]);
		QThreadBarrier_Index ^= 1;
		/* At this point all the qthreads for this target have calculated their results.
		 * Now each of the qthreads for this target must enter the target thread barrier for all the other
		 * targets to get their results calculated. 
		 * When all the targets have arrived here, then target thread 0, qthread 0 will display the results
		 * for each of the targets and each of the qthreads in sequence.
		 */
		if ((!(xgp->global_options & RX_NOBARRIER)) && (xgp->number_of_iothreads > 1)) {
			xdd_barrier(&xgp->thread_barrier[barrierindx]);
			barrierindx ^= 1; /* toggle the barrier index */
		}
		/* Have target 0 qthread 0 display results for each qthread for this pass if VERBOSE output was requested */
		/***************** This is where we need to calculate the deskewed results for this pass ******************/
		/* The deskew stuff is a bit tricky. For each of the targets, and it is assumed that there are ALOT of targets,
		 * we need to find the following:
			1) Time that each target started 
			2) The number of full requests made before the last target start time
			3) The start and end times of the I/O that straddles the last target start time. There is a
			   possibility that a target is in between I/O operations in which case it has no I/O straddling
			   the last target start time. This is fine.
		    4) The number of bytes transferred for the I/O that straddled the last target start time.
		   All this information is in the timestamp buffer for each of the targets. 
		   What we need to figure out at this point is the following:
		   1) The total number of bytes transferred during all previous I/O operations. It is possible that
			   there were no previous I/O operations and that the first I/O operation is the one straddling the
			   last target start time. That is ok because the previous bytes transferred will simply be 0.
		   2) The total number of 
		 */
		if ((p->mynum == 0)&& (p->myqnum == 0)) {
			for (i=0; i<xgp->number_of_targets; i++) {
				tp = &xgp->ptds[i]; /* Current target pointer */
				qp = tp; /* Qthread pointer */
				tp->perpass.open_start_time = tp->open_start_time;
				tp->perpass.open_end_time = tp->open_end_time;
				tp->perpass.open_time = (tp->open_end_time - tp->open_start_time); // Calculate the target open time
				tp->perpass.passes_completed++;

				/* Display information for each qthread in order. since this is qthread 0 then we start with p*/
				while (qp) {
					xdd_display_qthread_pass_results(qp,xgp->output, xgp->csvoutput, xgp->global_options&(RX_QTHREAD_INFO|RX_REALLYVERBOSE));
					qp = qp->nextp;
				}
				/* Display pass results for all queue threads combined into one */
				xdd_display_target_pass_results(tp,xgp->output, xgp->csvoutput, xgp->global_options&(RX_VERBOSE|RX_REALLYVERBOSE));
			}
			if (xgp->global_options & RX_DESKEW) {
				xdd_deskew();
			}
		}
		/* Wait for target 0 qthread 0 to display all intermediate results */
		if ((!(xgp->global_options & RX_NOBARRIER)) && (xgp->number_of_iothreads > 1)) {
			xdd_barrier(&xgp->thread_barrier[barrierindx]);
			barrierindx ^= 1; /* toggle the barrier index */
		}
		/* Wait for all the other qthreads to reach this point before we continue 
		 * At this point we are waiting for qthread 0 to complete the display of the results for this pass 
		 */
		xdd_barrier(&xgp->QThread_Barrier[p->mynum][QThreadBarrier_Index]);
		QThreadBarrier_Index ^= 1;
		/* Check to see if the run time has been exceeded - if so, then exit this loop.
		 * Otherwise, if there was a run time specified and we have not reached that run time
		 * and this is the last pass, then add one to the pass count so that we keep going.
		 */
		if (xgp->runtime > 0) {
			if (xgp->run_ring) /* This is the alarm that goes off when the total run time specified has been exceeded */
				break; /* Time to leave */
			else if (p->current_pass == xgp->passes) /* Otherwise if we just finished the last pass, we need to keep going */
				xgp->passes++;
		}
		/* Insert a delay of "passdelay" seconds if requested */
		if (xgp->passdelay > 0)
			sleep(xgp->passdelay);
		/* Re-randomize the seek list if requested */
		if (p->target_options & RX_PASSRANDOMIZE) {
			p->seekhdr.seek_seed++;
			xdd_init_seek_list(p);
		}

		/* Add an offset to each location in the seeklist */
		for (i = 0; i < p->total_ops; i++)
			p->seekhdr.seeks[i].block_location += p->pass_offset;

		/* Close current file, create a new target file, and open the new (or existing) file is requested */
		if (((p->target_options & RX_CREATE_NEW_FILES) || (p->target_options & RX_REOPEN) || (p->target_options & RX_RECREATE)) && (p->myqnum == 0)){
			close(p->fd); // Close the existing target
			if (p->target_options & RX_CREATE_NEW_FILES) { // Create a new file name for this target
				sprintf(p->targetext,"%08d",p->current_pass+1);
			}
			// Check to see if this is the last pass - in which case do not create a new file because it will not get used
			if (p->current_pass == xgp->passes)
				continue;
                        // If we need to "recreate" the file for each pass then we should delete it here before we re-open it 
			if (p->target_options & RX_RECREATE)	
#ifdef WIN32
				DeleteFile(p->target);
#else
#if (HPSS)
				if (p->target_options & RX_HPSS)
		 			hpss_Unlink(p->target);
				else unlink(p->target);
#else
				unlink(p->target);
#endif
#endif
			/* open the old/new/recreated target file */
			p->fd = xdd_open_target(p);
			if ((unsigned int)p->fd == -1) { /* error openning target */
				fprintf(xgp->errout,"%s: Aborting I/O for target %d due to open failure\n",xgp->progname,p->mynum);
				fflush(xgp->errout);
				xgp->abort_io = 1;
			}
		} // End of section that creates new target files and/or re-opens the target

	} /* end of FOR loop p->current_pass */
	///////////////////////////////////////////////All Passes have completed ///////////////////////////////////////////////
	if (p->target_options & RX_READAFTERWRITE) {
			if (xgp->global_options & RX_REALLYVERBOSE)
				fprintf(stderr,"closing socket %d, recvd msgs = %d sent msgs = %d\n",p->raw_sd, p->raw_msg_recv, p->raw_msg_sent);
			close(p->raw_sd);
	}
	/* Remember the last end time  for this qthread */
	p->qthread_results.end_time = p->perpass.end_time;
	p->qthread_results.latest_end_time = p->perpass.end_time;
	/* At this point qthread_results contains the cumulative results for this qthread over all passes.
	 * It is now time to calculate the combined rates for this qthread for all passes.
	 */
	xdd_calculate_rates(&p->qthread_results);
	/* Calculate the CPU utilization stats for this qthread */
	p->qthread_results.us_time = p->qthread_results.user_time + p->qthread_results.system_time;
	if (p->qthread_results.wall_clock_time > 0.0) 
		p->qthread_results.percent_cpu = (p->qthread_results.us_time / p->qthread_results.elapsed) * 100.0;
	else p->qthread_results.percent_cpu = 0.0;
	/* Display the results for this Target/Qthread */
	/* So here's the deal: QThread 0 for any target is special. It will be the collection
	 * point for all the results from all the Qthreads for a particular target.
	 * Since there are various reporting options, it is necessary to clarify what is being displayed.
	 * In the case of a "target aggregate" the results from each of the QThreads will be aggregated
	 * into a single combined output line. This is always reported.
	 * In the case of "individual QThreads" the results from each QThread will be displayed.
	 * In order to implement this correctly, all the QThreads need to come together at this point
	 * before the results can be aggregated. This is done by using the QThread Display Barrier - the point
	 * at which all the QThreads for a particular target converge when they have finished all their
	 * respective I/O operations. Once they have reached this barrier, they all enter the next barrier
	 * called the QThread Continuation Barrier which is where they all wait until the QThread 0 is 
	 * finished aggegating and displaying the results information. Once QThread 0 enters this barrier
	 * they are all free to continue to the next pass. 
	 */
	/* Wait for all other QThreads to reach this point before displaying information.
	 * Information for all QThreads is displayed by QThread 0 no matter what.
	 * Wait for all the other qthreads to reach this point before we continue 
	 * At this point we are waiting for all qthreads to complete before we display of the Average results for all passes 
	 */
	xdd_barrier(&xgp->QThread_Barrier[p->mynum][QThreadBarrier_Index]);
	QThreadBarrier_Index ^= 1;
	/* At this point All QThreads have there results in order and QThread 0 will display individual qthread summary
	 * information if requested. Qthread 0 will also display the combined qthread information for all qthreads overall
	 * passes for this target. This is the "target" average results.
	 * All other QThreads will simply wait at the QThread Barrier until QThread 0 is done displaying 
	 * the results. 
	 * The cumulative results for each qthread over all passes has been put in the "qthread_results"
	 * structure for the respective qthread.
	 * The xdd_display_target_average_results() routine will combine all the qthread_results structures into
	 * target_results (located in qthread 0 ptds) for this target. (The target_results structures in qthreads 1 thru n
	 * are not used.)
	 */
	if ((!(xgp->global_options & RX_NOBARRIER)) && (xgp->number_of_iothreads > 1)) {
		xdd_barrier(&xgp->thread_barrier[barrierindx]);
		barrierindx ^= 1; /* toggle the barrier index */
	}
	/* Have target 0 qthread 0 display results for each qthread for this pass if VERBOSE output was requested */
	if ((p->mynum == 0)&& (p->myqnum == 0)) {
		for (i=0; i<xgp->number_of_targets; i++) {
			tp = &xgp->ptds[i]; /* Current target pointer */
			qp = tp; /* Qthread pointer */
			/* Display information for each qthread in order. since this is qthread 0 then we start with p*/
			while (qp) {
				xdd_display_qthread_average_results(qp, xgp->output, xgp->csvoutput,xgp->global_options&(RX_QTHREAD_INFO|RX_REALLYVERBOSE));
				qp = qp->nextp;
			}
			/* Display pass results for all queue threads combined into one */
			xdd_display_target_average_results(tp,xgp->output, xgp->csvoutput, xgp->global_options&(RX_VERBOSE|RX_REALLYVERBOSE));
		}
	}
	/* Wait for target 0 qthread 0 to display all intermediate results */
	if ((!(xgp->global_options & RX_NOBARRIER)) && (xgp->number_of_iothreads > 1)) {
		xdd_barrier(&xgp->thread_barrier[barrierindx]);
		barrierindx ^= 1; /* toggle the barrier index */
	}
	p->thread_complete = 1; /* indicate that this thread is in fact complete */
	/* Wait for all target threads to complete before writing ts reports */
	xdd_barrier(&xgp->prefinal_barrier);
	/* if I am target 0 qthread 0 then I get to collect the results from all the targets and put them into 
	 * the final results structure and display them.
	 */
	if ((p->mynum == 0) && (p->myqnum == 0)) {
		xdd_init_results(&final_results);
		if (p->rwratio == 1.0) 
			strcpy(p->qthread_results.optype, "read");
		else if (p->rwratio == 0.0)
			strcpy(p->qthread_results.optype, "write");
		else strcpy(p->qthread_results.optype, "mixed");

		final_results.earliest_start_time = p->qthread_results.earliest_start_time;
		final_results.latest_end_time = p->qthread_results.latest_end_time;
		/* Scan all the targets qthread results looking for the earliest start time and the latest end time */
		for (j=0; j< xgp->number_of_targets; j++) {
			tp = &xgp->ptds[j];
			qp = tp;
			while (qp) {
				if (qp->qthread_results.earliest_start_time < final_results.earliest_start_time)
					final_results.earliest_start_time = qp->qthread_results.earliest_start_time;
				if (qp->qthread_results.latest_end_time > final_results.latest_end_time)
					final_results.latest_end_time = qp->qthread_results.latest_end_time;
				qp = qp->nextp;
			}
		}
		final_results.elapsed = ((final_results.latest_end_time - final_results.earliest_start_time)/TRILLION);
		if (final_results.elapsed < 0.0) 
			final_results.elapsed *= -1.0;
		/* Now combine the results from all the targets into one. */
		for (i = 0; i < xgp->number_of_targets; i++) {
			xdd_combine_results(&xgp->ptds[i].target_results,&final_results);
		}
		final_results.percent_cpu = (final_results.us_time / final_results.elapsed) * 100.0;

		xdd_display_results(
		"         Combined",  // id
		xgp->number_of_targets, // target 
		xgp->number_of_iothreads,//queue
		final_results.bytes, // bytes, 
		final_results.ops,  // ops, 
		final_results.elapsed, //time, 
		final_results.avg_rate, // rate, 
		final_results.avg_iops, // iops 
		final_results.avg_latency, // latency, 
		final_results.percent_cpu, // percentcpu 
		final_results.optype, // Operation Type 
		final_results.reqsize, // Average Request Size
	        (final_results.open_time+500000)/1000000, // Average Open Time
		final_results.start_time - p->base_time, // Start time
		final_results.end_time - p->base_time,   // End time
		xgp->output,xgp->csvoutput,xgp->global_options&(RX_VERBOSE|RX_REALLYVERBOSE));
		if (xgp->global_options & RX_COMBINED) {
			xdd_display_csv_results(
			"Combined",  // id
			xgp->number_of_targets, // target 
			xgp->number_of_iothreads,//queue
			final_results.bytes, // bytes, 
			final_results.ops,  // ops, 
			final_results.elapsed, //time, 
			final_results.avg_rate, // rate, 
			final_results.avg_iops, // iops 
			final_results.avg_latency, // latency, 
			final_results.percent_cpu, // percentcpu, 
			final_results.optype, // Operation Type, 
			final_results.reqsize, // Average Request Size,
			(final_results.open_time+500000)/1000000, // Average Open Time
			final_results.start_time - p->base_time, // Start time
			final_results.end_time - p->base_time,   // End time
			xgp->combined_output,xgp->global_options&(RX_VERBOSE|RX_REALLYVERBOSE));
		}
		for (j=0; j< xgp->number_of_targets; j++) {
			tp = &xgp->ptds[j];
			qp = tp;
			while (qp) {
				/* Display and write the time stamping information if requested */
				if (qp->ts_options & (TS_ON | TS_TRIGGERED)) {
					if (qp->timestamps > qp->ttp->tt_size) 
						qp->ttp->numents = qp->ttp->tt_size;
					else qp->ttp->numents = qp->timestamps;
					if (qp->thread_complete == 0) {
						fprintf(xgp->errout,"%s: Houston, we have a problem... thread for Target %d Q %d is not complete and we are trying to dump the time stamp file!\n", xgp->progname, qp->mynum, qp->myqnum);
						fflush(xgp->errout);
					}
					xdd_ts_reports(qp);  /* generate reports if requested */
					xdd_ts_write(qp); 
					xdd_ts_cleanup(qp->ttp); /* call this to free the TS table in memory */
				}
				qp = qp->nextp;
			}
		} /* end of FOR loop that processes all time stamp reports for all qthreads in all targets */
		if ((p->target_options & (RX_VERIFY_LOCATION | RX_VERIFY_CONTENTS)) && (p->rwratio > 0.0)) {
			if (final_results.compare_errors == 0)
				fprintf(xgp->output, "No errors reported on Sequenced Data Compare operation\n");
			else
#ifdef WIN32
			fprintf(xgp->output, "There were %I64d Sequenced Data Miscompares detected. Refer to the error output for details\n",final_results.compare_errors);
#else
			fprintf(xgp->output, "There were %lld Sequenced Data Miscompares detected. Refer to the error output for details\n",final_results.compare_errors);
#endif
		}
	}
#ifdef WIN32
	/* No Close equivalent for NT */
#else
#if (HPSS)
	if (p->target_options & RX_HPSS)
		 hpss_Close(p->fd);
	else close(p->fd);
#else
	close(p->fd);
#endif
#endif
	if ((p->target_options & RX_DELETEFILE) && (p->target_options & RX_REGULARFILE)) {
#ifdef WIN32
		DeleteFile(p->target);
#else
#if (HPSS)
	if (p->target_options & RX_HPSS)
		 hpss_Unlink(p->target);
	else unlink(p->target);
#else
		unlink(p->target);
#endif
#endif
		fprintf(xgp->errout,"%s: File %s removed\n",xgp->progname,p->target);
		fflush(xgp->errout);
	}
	for (i = 0; i < p->queue_depth; i++) {
		xdd_unlock_memory(p->rwbuf_save, p->iosize, "IOBuffer");
	}
#if (AIX)
	if (p->target_options & RX_SHARED_MEMORY) {
		if (shmctl(p->rwbuf_shmid, IPC_RMID, NULL)  == -1)
			fprintf(xgp->errout,"%s: Cannot Remove SHMID for Target %d Q %d\n",xgp->progname, p->mynum, p->myqnum);
		else if (xgp->global_options & RX_REALLYVERBOSE)
				fprintf(xgp->output,"Shared Memory ID removed, shmid=%d\n",p->rwbuf_shmid);
	}
#endif
	xdd_barrier(&xgp->final_barrier);
    return(0);
} /* end of xdd_io_thread() */
/*----------------------------------------------------------------------------*/
/* This is the main entry point for xdd. This is where it all begins and ends.
 */
int32_t
main(int32_t argc,char *argv[]) {
	int32_t i,j,q;
	int32_t status;
	pclk_t base_time; /* This is time t=0 */
	pclk_t tt; 
	pthread_t heartbeat_thread;
	ptds_t *p;
	char *c;

//fprintf(stderr,"Allocating %d bytes of memory for globals\n",sizeof(struct xdd_globals));
    xgp = (xdd_globals_t *)malloc(sizeof(struct xdd_globals));
    if (xgp == 0) {
        fprintf(stderr,"%s: Cannot allocate %d bytes of memory for global variables!\n",argv[0], sizeof(struct xdd_globals));
	perror("Reason");
        exit(1);
    }
    xdd_init_globals();

	xgp->canceled = 0;
	/* Parese the input arguments */
//fprintf(stderr,"parsing args...\n");
	xgp->argc = argc; // remember the original arg count
	xgp->argv = argv; // remember the original argv 
	xdd_parse(argc,argv);
	/* optimize runtime priorities and all that */
//fprintf(stderr,"schedule options...\n");
	xdd_schedule_options();
	/* initialize the signal handlers */
//fprintf(stderr,"init signals...\n");
	xdd_init_signals();
	/* Init all the necessary barriers */
//fprintf(stderr,"init barriers...\n");
	xdd_init_all_barriers();
#ifdef WIN32
	/* Init the ts serializer mutex to compensate for a Windows bug */
	xgp->ts_serializer_mutex_name = "ts_serializer_mutex";
	ts_serializer_init(&xgp->ts_serializer_mutex, xgp->ts_serializer_mutex_name);
#endif
#if ( IRIX )
	/* test if allowed this many processes */
	if (xgp->number_of_iothreads > prctl(PR_MAXPROCS)) {
			fprintf(xgp->errout, "Can only utilize %d processes per user\n",prctl(PR_MAXPROCS));
			fflush(xgp->errout);
			xdd_destroy_all_barriers();
			exit(1);
	}
#endif 
//fprintf(stderr,"initializing clocks\n");
	/* initialize the clocks */
	pclk_initialize(&tt);
	if (tt == PCLK_BAD) {
			fprintf(xgp->errout, "%s: Cannot initialize the picosecond clock\n",xgp->progname);
			fflush(xgp->errout);
			xdd_destroy_all_barriers();
			exit(1);
	}
	pclk_now(&base_time);
	/* Init the Global Clock */
	xdd_init_global_clock(&xgp->ActualLocalStartTime);
	/* display configuration information about this run */
	xdd_config_info();
	/* Start up all the threads */
	/* The basic idea here is that there are N targets and each target can have X instances ( or queue threads as they are referred to)
	 * The "ptds" array contains pointers to the main ptds's for the primary targets.
	 * Each target then has a pointer to a linked-list of ptds's that constitute the targets that are used to implement the queue depth.
     * The thread creation process is serialized such that when a thread is created, it will perform its initialization tasks and then
     * enter the "serialization" barrier. Upon entering this barrier, the *while* loop that creates these threads will be released
     * to create the next thread. In the meantime, the thread that just finished its initialization will enter the main thread barrier 
     * waiting for all threads to become initialized before they are all released. Make sense? I didn't think so.
	 */
	for (j=0; j<xgp->number_of_targets; j++) {
		p = &xgp->ptds[j]; /* Get the ptds for this target */
		q = 0;
		for (i=0; i<=1; i++) {
				sprintf(xgp->QThread_Barrier[j][i].name,"QThread_T%d_%d",j,i);
				status = xdd_init_barrier(&xgp->QThread_Barrier[j][i], 
					p->queue_depth, xgp->QThread_Barrier[j][i].name);
				}
		while (p) { /* create a thread for each ptds for this target - the queue depth */
			/* Create the new thread */
			p->base_time = base_time;
			status = pthread_create(&p->thread, NULL, xdd_io_thread, p);
			if (status) {
				fprintf(xgp->errout,"%s: Error creating thread %d",xgp->progname, j);
				fflush(xgp->errout);
				perror("Reason");
				xdd_destroy_all_barriers();
				exit(1);
			}
			p->mynum = j;
			p->myqnum = q;
			p->total_threads = xgp->number_of_iothreads;
			/* Wait for the previous thread to complete initialization and then create the next one */
			xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
            // If the previous target *open* fails, then everything needs to stop right now.
            if (xgp->abort_io == 1) { 
		        fprintf(xgp->errout,"%s: xdd thread %d aborting due to previous open failure\n",xgp->progname,p->mynum);
		        fflush(xgp->errout);
				xdd_destroy_all_barriers();
				exit(1);
	    }
			q++;
			p = p->nextp; /* get next ptds in the queue for this target */
		}
	} /* end of FOR loop that starts all procs */
	/* start a heartbeat monitor if necessary */
	if (xgp->heartbeat) {
		status = pthread_create(&heartbeat_thread, NULL, xdd_heartbeat, (void *)j);
		if (status) {
			fprintf(xgp->errout,"Error creating heartbeat monitor thread");
			fflush(xgp->errout);
		}
	}

    /* Display the results banner */
    xdd_display_results_header();

	/* Wait for all children to finish */
	xdd_barrier(&xgp->final_barrier);
	xgp->current_time_for_this_run = time(NULL);
	c = ctime(&xgp->current_time_for_this_run);
	fprintf(xgp->output,"Ending time for this run, %s\n",c);
	if (xgp->csvoutput)
		fprintf(xgp->csvoutput,"Ending time for this run, %s\n",c);

	/* Cleanup the semaphores and barriers */
	sleep(1); /* Wait for all children processes to exit */
	xdd_destroy_all_barriers();
	/* Time to leave... sigh */
	return(0);
} /* end of main() */
