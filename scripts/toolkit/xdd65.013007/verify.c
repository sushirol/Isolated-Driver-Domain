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
 * This file contains the subroutines necessary to perform data verification
 * This is new as of version 6.6
 */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_verify_checksum() - Verify data checksum of the data buffer  
 * Returns the number of miscompare errors.
 */
int32_t
xdd_verify_checksum(ptds_t *p, int32_t current_op) {
	fprintf(xgp->errout, "NOT IMPLEMENTED YET\n");
	return(0);
} // end of xdd_verify_checksum()

/*----------------------------------------------------------------------------*/
/* xdd_verify_hex() - Verify hex data pattern in the data buffer  
 * Returns the number of miscompare errors.
 * This routine assumes that the specified hex data pattern and replication 
 * factor has been previously written to the media that was just read and 
 * is being verified. 
 * It is further assumed that the data pattern and data pattern lenggth
 * are in p->data_pattern and p->data_pattern_length respectively. This is
 * done by the datapattern function in the parse.c file. If the target_option
 * of "RX_REPLICATE_PATTERN" was specified as well, then the data comparison is
 * made throughout the data buffer. Otherwise only the first N bytes are compared
 * against the data pattern where N is equal to p->data_pattern_length. Cool, huh?
 */
int32_t
xdd_verify_hex(ptds_t *p, int32_t current_op) {
	int32_t i;
	int32_t errors;
	int32_t remaining;
	int32_t offset;
	unsigned char *patternp, *bufferp;


	if (p->target_options & RX_REPLICATE_PATTERN) 
		remaining = p->actual_iosize;
	else remaining = p->data_pattern_length;

	offset = 0;
	bufferp = p->rwbuf;
	errors = 0;
	while (remaining) {
		patternp = p->data_pattern;
		for (i=0; i<p->data_pattern_length; i++, patternp++, bufferp++) {
			if (*patternp != *bufferp) {
#ifdef WIN32
				fprintf(xgp->errout,"%s [%d]: Content mismatch on target %s at %d bytes into block %I64u, expected 0x%02x, got 0x%02x\n",
					xgp->progname, p->mynum, p->target, offset, p->my_current_byte_location/p->block_size, *patternp, *bufferp);
#else
				fprintf(xgp->errout,"%s [%d]: Content mismatch on target %s at %d bytes into block %llu, expected 0x%02x, got 0x%02x\n",
					xgp->progname, p->mynum, p->target, offset, p->my_current_byte_location/p->block_size, *patternp, *bufferp);
#endif
				errors++;
			}
			offset++;
			remaining--;
			if (remaining <= 0) break;
		}
	}
	return(errors);
} // end of xdd_verify_hex()
/*----------------------------------------------------------------------------*/
/* xdd_verify_sequence() - Verify data contents  of a sequenced data pattern 
 * Returns the number of miscompare errors.
 * The 8-byte sequence number data patter is specified as "-datapattern sequenced". This will cause xdd to write a
 * sequence of 8-byte integers that start at 0 and increment by 8 until the end of the buffer is reached. 
 * The hex representation of the sequenced data patter would look like so:
 *      00000000 00000008 00000010 00000018...
 */
int32_t
xdd_verify_sequence(ptds_t *p, int32_t current_op) {
	int32_t  i,j;
	int32_t  errors;
	uint64_t expected_data;
	uint64_t *uint64p;
	unsigned char *ucp;        /* A temporary unsigned char pointer */
 

    uint64p = (uint64_t *)p->rwbuf;
	errors = 0;
	for (i = 0; i < p->actual_iosize; i+=(sizeof(p->my_current_byte_location))) {
		expected_data = p->my_current_byte_location + i;
		if (*uint64p != expected_data) {
#ifdef WIN32
			fprintf(xgp->errout,"%s [%d]: Data Buffer Content mismatch on target %s at %d bytes into block %I64u, ",
				xgp->progname, p->mynum, p->target, i, p->my_current_byte_location/p->block_size);
#else
			fprintf(xgp->errout,"%s [%d]: Data Buffer Content mismatch on target %s at %d bytes into block %llu, ",
				xgp->progname, p->mynum, p->target, i, p->my_current_byte_location/p->block_size);
#endif
			fprintf(xgp->errout, "expected 0x");
			for (j=0, ucp=(unsigned char *)&expected_data; j<sizeof(uint64_t); j++, ucp++) {
				fprintf(xgp->errout, "%02x",*ucp);
			}
			fprintf(xgp->errout, ", got 0x");
			for (j=0, ucp=(unsigned char *)uint64p; j<sizeof(uint64_t); j++, ucp++) {
				fprintf(xgp->errout, "%02x",*ucp);
			}
			fprintf(xgp->errout, "\n");
			errors++;
		}
		uint64p++;
	} // end of FOR loop that looks at all locations 
	return(errors);
} // end of xdd_verify_sequence() 

/*----------------------------------------------------------------------------*/
/* xdd_verify_singlechar() - Verify data contents of a single character data pattern 
 * Returns the number of miscompare errors. 
 * The single-byte data pattern is specified simply by giving the -datapattern a single character to write to the device. 
 * If that same character is specified for a read operation with the -verify option then that character will be compared with the
 * contents of the I/O buffer for every block read.
 */
int32_t
xdd_verify_singlechar(ptds_t *p, int32_t current_op) {
	int32_t  i;
	int32_t  errors;
	unsigned char *ucp;
 
	ucp = p->rwbuf;
	errors = 0;
	for (i = 0; i < p->actual_iosize; i++) {
		if (*ucp != *(p->data_pattern)) {
#ifdef WIN32
		fprintf(xgp->errout,"%s [%d]: Content mismatch on target %s at %d bytes into block %I64u, expected 0x%02x, got 0x%02x\n",
			xgp->progname, p->mynum, p->target, i, p->my_current_byte_location/p->block_size, *(p->data_pattern), *ucp);
#else
		fprintf(xgp->errout,"%s [%d]: Content mismatch on target %s at %d bytes into block %llu, expected 0x%02x, got 0x%02x\n",
			xgp->progname, p->mynum, p->target, i, p->my_current_byte_location/p->block_size, *(p->data_pattern), *ucp);
#endif
		errors++;
		} /* End printing error message */
		ucp++;
	} // end of FOR statement that looks at all bytes

	return(errors);

} // end of xdd_verify_singlechar() 

/*----------------------------------------------------------------------------*/
/* xdd_verify_contents() - Verify data contents  
 * Returns the number of miscompare errors.
 * There are various kinds of data patterns that xdd can read back for comparison. 
 * The user is responsible for using xdd to write the desired data pattern to the device 
 * and then request the proper verification / data pattern.
 * The  data patterns currently supported are: single byte data, hex digits, ascii strings, and 8-byte sequence numbers. 
 * There is a separate subroutine in this file that handles the verification for each type of data pattern.
 * The subroutine names are obvious. If not, you should not be reading this.
 */
int32_t
xdd_verify_contents(ptds_t *p, int32_t current_op) {
	int32_t  errors;

	errors = 0;
	/* Verify the contents of the buffer is equal to the specified data pattern */
	if (p->target_options & RX_SEQUENCED_PATTERN) { // Lets look at a sequenced data pattern
		errors = xdd_verify_sequence(p, current_op);
		return(errors);
	}

	if (p->target_options & RX_HEX_PATTERN) { // Lets look at a HEX data pattern
		errors = xdd_verify_hex(p, current_op);
		return(errors);
	}

	if (p->target_options & RX_SINGLECHAR_PATTERN) { // Lets look at a single character data pattern
		errors = xdd_verify_singlechar(p, current_op);
		return(errors);
	}

	// If we get here then the data pattern was either not specified or the data pattern type was not recognized.
	fprintf(xgp->errout, "%s [%d]xdd_verify_contents: Data verification type not understood for target %s. No verification possible.\n",
				xgp->progname, p->mynum, p->target);
	return(0);
	
} // end of xdd_verify_contents()  

/*----------------------------------------------------------------------------*/
/* xdd_verify_location() - Verify data location 
 * This routine gets the current bytes location that is located in the first
 * 8-bytes of the rw buffer and compares it to the current byte location that
 * the calling routine specified in the ptds->my_current_byte_location. If the
 * two do not match then we are not in Kansas anymore. Print an error message
 * and return a 1. Otherwise, everything is peachy, simply return a 0.
 * Returns the number of miscompare errors - 0 or 1 in this case.
 */
int32_t
xdd_verify_location(ptds_t *p, int32_t current_op) {
	int32_t  errors;
	uint64_t current_position;

	errors = 0;
	current_position = *(uint64_t *)p->rwbuf;
	if (current_position != p->my_current_byte_location) {
		errors++;
#ifdef WIN32
		fprintf(xgp->errout,"%s [%d]: Data Buffer Sequence mismatch on target %s - expected %I64u, got %I64u\n",
			xgp->progname, p->mynum, p->target, p->my_current_byte_location, current_position);
#else
		fprintf(xgp->errout,"%s [%d]: Data Buffer Sequence mismatch on target %s - expected %llu, got %llu\n",
			xgp->progname, p->mynum, p->target, p->my_current_byte_location, current_position);
#endif
		fflush(xgp->errout);
	}
	return(errors);
} // end of xdd_verify_location() 

/*----------------------------------------------------------------------------*/
/* xdd_verify() - Verify data location and/or contents  
 * Returns the number of miscompare errors.
 */
int32_t
xdd_verify(ptds_t *p, int32_t current_op) {
	int32_t  errors;


	// We only do verification of data if the last operation was a read. Otherwise, just return.
	if (p->seekhdr.seeks[current_op].operation =! RX_READ){
		fprintf(xgp->errout, "%s [%d] xdd_verify: Data verification for target %s during a write operation is not possible. No verification performed.\n",
				xgp->progname, p->mynum, p->target);
		return(0);
	}

   /* Since the last operation was a read operation check to see if a sequenced data pattern
	* was specified. If so, then we need to verify that what we read has the correct 
	* sequence number(s) in it.
	*/
	if (!(p->target_options & (RX_VERIFY_CONTENTS | RX_VERIFY_LOCATION))) { // If we don't need to verify location or contents of the buffer, then just return.
		fprintf(xgp->errout, "%s [%d] xdd_verify: Data verification type <location or contents> not specified. No verification performed.\n",
				xgp->progname, p->mynum, p->target);
		return(0);
	}

	// Looks like we need to verify something...
	if (p->target_options & RX_VERIFY_LOCATION) /* Assumes that the data pattern was sequenced. If not, there will be LOTS o' errors. */
		 errors = xdd_verify_location(p, current_op);
	else errors = xdd_verify_contents(p, current_op);

	return(errors);
} /* End of xdd_verify() */