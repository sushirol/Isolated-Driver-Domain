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
 * This file contains the subroutines necessary to parse the command line
 * arguments  set up all the global and target-specific variables.
 */

#include "xdd.h"
/* -------------------------------------------------------------------------------------------------- */
// The xdd Function Table
//   This table contains entries of the following structure as defined in xdd.h:
//            struct xdd_func {
//	              char    *func_name;     /* name of the function */
//	              char    *func_alt;      /* Alternate name of the function */
//                int     (*func_ptr)(int32_t argc, char *argv[]);      /* pointer to the function */
//                int     argc;           /* number of arguments */
//                char    *help;          /* help string */
//                char    *ext_help[5];   /* Extented help strings */
//            };
xdd_func_t  xdd_func[] = {
    {"align", "al",
            xddfunc_align,      
            1,  
            "  -align [target <target#>] <#bytes>\n",  
            {"    Align memory on an #-byte boundary - should be an even number\n", 
            0,0,0,0}},
    {"blocksize", "bs",
            xddfunc_blocksize,  
            1,  
            "  -blocksize [target <target#>] <#bytes/block>\n",  
            {"    Specifies the size of a single 'block'.\n", 
            0,0,0,0}},
    {"combinedout", "combo",
            xddfunc_combinedout,
            1,  
            "  -combinedout <filename>\n",  
            {"    Will direct a copy of just the Combined output lines to be 'appended' to the specified file\n", 
            0,0,0,0}},
    {"createnewfiles",  "cnf",
            xddfunc_createnewfiles,  
            1,  
            "  -createnewfiles [target <target#>]\n",  
            {"    Will continually create new target files by appending a number to the target file name for each new file\n", 
            0,0,0,0}},
    {"csvout", "csvo",
            xddfunc_csvout,     
            1,  
            "  -csvout <filename>\n",  
            {"    Will direct Comma Separated Values output to the specified file\n", 
            0,0,0,0}},
    {"datapattern", "dp",
            xddfunc_datapattern,    
            1,  
            "  -datapattern [target <target#>] <c> | random | sequenced | ascii <asciistring> | hex <hexdigits> | replicate\n",  
            {"    -datapattern 'c' will use the character c as the data pattern to write\n",
             "     If the word 'random' is specified for the pattern then a random pattern will be generated\n\
                   If the word 'sequenced' is specified for the pattern then a sequenced number pattern will be generated\n\
			       If the word 'ascii' is specified then the following string is used as a single pattern\n\
			       If the word 'hex' is specified then the following hex characters <0-9,a-f or A-F> are used as the pattern\n\
			       If the word 'replicate' is specified then whatever pattern was specified is replicated throughout the buffer\n",
             "     Default data pattern is all binary 0's\n",
             0}},
    {"delay", "delay",       
            xddfunc_delay,      
            1,  
            "  -delay #seconds\n",  
            {"    Specifies the number of seconds to delay between passes\n", 
            0,0,0,0}},
    {"deletefile",  "del",
            xddfunc_deletefile,  
            1,  
            "  -deletefile [target <target#>]\n",  
            {"    Will delete the target file upon completion IFF the target is a regular file\n", 
            0,0,0,0}},
    {"deskew", "deskew",
            xddfunc_deskew,     
            1,  
            "  -deskew\n",  
            {"    Will cause the calculated bandwidth and IOPs to be deskewed before being reported\n", 
            0,0,0,0}},
    {"devicefile",  "devf",
            xddfunc_devicefile, 
            1,  
            "  -devicefile\n",  
            {"    Indicates that the file being accessed is a device - Windows only", 
            0,0,0,0}},
    {"directio",  "dio",
            xddfunc_dio,        
            1,  
            "  -dio [target <target#>]\n",  
            {"    Will use DIRECTIO on targets that are files\n", 
            0,0,0,0}},
    {"errout", "eo",
            xddfunc_errout,     
            1,  
            "  -errout <filename>\n",  
            {"    Will direct all error output to the specified file\n", 
            0,0,0,0}},
    {"fullhelp", "exthelp",
            xddfunc_fullhelp,   
            1,  
            "  -fullhelp\n",  
            {"    Extended Help - displays all kinds of unsful information", 
            0,0,0,0}},
    {"heartbeat", "hb",
            xddfunc_heartbeat,  
            1,  
            "  -heartbeat #\n",  
            {"    Will print out running counters every # seconds \n", 
            0,0,0,0}},
    {"identifier",   "id",
            xddfunc_id, 
            1,  
            "  -id \"string\" | commandline \n",  
            {"    Specifies an arbitrary ID string to place in the time stamp file and printed on stdout\n", 
            0,0,0,0}},
    {"kbytes",  "kb",
            xddfunc_kbytes,     
            1,  
            "  -kbytes [target <target#>] <#>\n",  
            {"    Specifies the number of 1024-byte blocks to transfer during a single pass\n", 
            0,0,0,0}},
    {"lockstep", "ls",
            xddfunc_lockstep,   
            1,  
            "  -lockstep <mastertarget#> <slavetarget#> <time|op|percent|mbytes|kbytes> # <time|op|percent|mbytes|kbytes># <wait|run> <complete|stop>\n",   
            {"  Where 'master_target' is the target that tells the slave when to do something.\n\
        'slave_target' is the target that responds to requests from the master.\n\
        'when' specifies when the master should tell the slave to do something.\n\
  The word 'when' should be replaced with the word: \n\
        'time', 'op', 'percent', 'mbytes', 'kbytes'.\n\
        'howlong' is either the number of seconds, number of operations, ...etc.\n\
        - The interval time in seconds <a floating point number> between task requests from the\n\
          master to the slave. i.e. if this number were 2.3 then the master would request\n\
          the slave to perform its task every 2.3 seconds until.\n\
        - The operation number that defines the interval on which the master will request\n\
          the slave to perform its task. i.e. if the operation number is set to 8 then upon\n\
          completion of every 8 <master> operations, the master will request the slave to perform its task.\n\
        - The percentage of operations that must be completed by the master before requesting\n\
          the slave to perform a task.\n\
        - The number of megabytes <1024*1024 bytes> or the number of kilobytes <1024 bytes>\n\
        'what' is the type of task the slave should perform each time it is requested to perform\n\
          a task by the master. The word 'what' should be replaced by:\n\
        'time', 'op', 'percent', 'mbytes', 'kbytes'.\n",
             "    'howmuch' is either the number of seconds, number of operations, ...etc.\n\
	    - The amount of time in seconds <a floating point number> the slave should run before\n\
          pausing and waiting for further requests from the master.\n\
        - The number of operations the slave should perform before pausing and waiting for\n\
          further requests from the master.\n\
        - The number of megabytes <1024*1024 bytes> or the number of kilobytes <1024 bytes>\n\
          the slave should transfer before pausing and waiting for further requests from the master.\n\
        'startup' is either 'wait' or 'run' depending on whether the slave should start running upon\n\
          invocation and perform a single task or if it should simply wait for the master to \n\
          request it to perform its first task. \n\
        'Completion' - in the event that the master finishes before the slave, then the slave will have\n\
          the option to complete all of its remaining operations or to just stop at this point.\n\
          This should be specified as either 'complete' or 'stop'.\n",
                         0,0,0}},
    {"lockstepoverlapped","lso",
            xddfunc_lockstep,
            1,  
            "  -lockstepoverlapped\n",  
            {"    See -lockstep", 
            0,0,0,0}},
    {"maxall", "maxall",
            xddfunc_maxall,     
            1,  
            "  -maxall\n",  
            {"    Will set maximumpriority and processlock\n", 
            0,0,0,0}},
    {"maxerrors", "maxerrors",
            xddfunc_maxerrors,  
            1,  
            "  -maxerrors #\n",  
            {"    Specifies the total number of errors to tollerate before giving up\n",
             "    The default is to NEVER give up\n",
             0,0,0}},
    {"maximumpriority", "maxpri",     
            xddfunc_maxpri,     
            1,  
            "  -maxpri\n",  
            {"    Will set the process to maximum priority\n", 
            0,0,0,0}},
    {"mbytes", "mb",
            xddfunc_mbytes,     
            1,  
            "  -mbytes [target <target#>] <#>\n",  
            {"    Specifies the number of 1024*1024-byte blocks to transfer in a single pass\n", 
            0,0,0,0}},
    {"minall", "minall",
            xddfunc_minall,     
            1,  
            "  -minall\n",  
            {"    Will set not lock memory, process, or reset process priority\n", 
            0,0,0,0}},
    {"nobarrier", "nb",
            xddfunc_nobarrier,  
            1,  
            "  -nobarrier\n",  
            {"    Turns off barriers before operation. All threads are free running\n", 
            0,0,0,0}},
    {"nomemlock", "nomlock",
            xddfunc_nomemlock,     
            1,  
            "  -nomemlock\n",  
            {"    Will set not lock memory\n", 
            0,0,0,0}},
    {"noproclock", "noplock",
            xddfunc_noproclock,     
            1,  
            "  -noproclock\n",  
            {"    Will set not lock process into memory\n", 
            0,0,0,0}},
    {"numreqs", "nr",
            xddfunc_numreqs, 
            1,  
            "  -numreqs [target <target#>] <#>\n",  
            {"    Specifies the number of requests of request_size to issue\n", 
            0,0,0,0}},
    {"operation", "op",
            xddfunc_operation,  
            1,  
            "  -operation [target <target#>] read|write\n",   
            {"   The operation is either 'read', 'write', -or- 'target # read' -or- 'target # write'\n", 
             "   The 'target # <op>' will cause the specified target to perform the specified operations\n",
             "   The default is 'read' for all targets unless otherwise specified\n",
             0,0}},
    {"output", "o",
            xddfunc_output,     
            1,  
            "  -output <filename>\n",  
            {"    Will direct all output to the specified file\n", 
            0,0,0,0}},
    {"passes", "p",
            xddfunc_passes,     
            1,  
            "  -passes #\n",  
            {"    Specifies the number of times to read mbytes -or- the number of times to issue 'numreqs' requests\n", 
            0,0,0,0}},
    {"passoffset",  "po",
            xddfunc_passoffset, 
            1,  
            "  -passoffset [target <target#>] <#blocks>\n",  
            {"    Specifies the number of blocks to offset between passes\n", 
            0,0,0,0}},
    {"preallocate", "pa",
            xddfunc_preallocate,
            1,  
            "  -preallocate [target <target#>] <#blocks>\n",  
            {"    Will preallocate # blocksize blocks before writing a file.\n", 
            0,0,0,0}},
    {"processlock", "plock",      
            xddfunc_plock,      
            1,  
            "  -processlock\n",  
            {"    Will lock the process in memory\n", 
            0,0,0,0}},
    {"processor", "pt",
            xddfunc_processor, 
            1,  
            "  -processor target# processor#\n",  
            {"    Specifies which processor xdd should run on for a particular target\n",
             "    Requires the processor number and the associated target number\n",
             0,0,0}},
    {"queuedepth", "qd",
            xddfunc_queuedepth, 
            1,  
            "  -queuedepth #cmds\n",   
            {"    Specifies the number of commands to queue on the target\n", 
            0,0,0,0}},
    {"qthreadinfo", "qtinfo",
            xddfunc_qthreadinfo,
            1,  
            "  -qthreadinfo\n",  
            {"    Will print out information about each of the queue threads \n", 
            0,0,0,0}},
    {"randomize", "rand",
            xddfunc_randomize,  
            1,  
            "  -randomize [target <target#>]\n",   
            {"    Will re-randomize the seek list between passes\n", 
            0,0,0,0}},
    {"readafterwrite","raw",
            xddfunc_readafterwrite,
            1,  
            "  -readafterwrite [target #] trigger <stat | mp> | lag <#> | reader <hostname> | port <#>\n",  
            {"    Specifies a reader and writer for doing read-after-writes to a single target", 
            0,0,0,0}},
    {"reallyverbose", "rv",
            xddfunc_reallyverbose, 
            1,  
            "  -reallyverbose\n",  
            {"    Displays way more information than you need to know", 
            0,0,0,0}},
    {"recreatefiles",  "recreate",
            xddfunc_recreatefiles,  
            1,  
            "  -recreatefiles [target <target#>]\n",  
            {"    Will recreate a new target file on each pass\n", 
            0,0,0,0}},
    {"reopen",  "reop",
            xddfunc_reopen,  
            1,  
            "  -reopen [target <target#>]\n",  
            {"    Will cause the target file to be closed at the end of each pass and re-opened at the beginning of each pass\n", 
            0,0,0,0}},
    {"reportthreshold","rept",
            xddfunc_report_threshold,
            1,
            "  -reportthreshold [target #] <#.#>\n",  
            {"    The report threshhold will report the byte location of the operation that exceeded the specified threshold time.\n", 
            0,0,0,0}},
    {"reqsize", "rs",
            xddfunc_reqsize,    
            1,  
            "  -reqsize [target <target#>] <#blocks>\n",  
            {"    Specifies the number of 'blocks' per operation where the block size is defined by the '-blocksize' option\n",
             "    If the request size is specified in the format '-reqsize target <targetnumber> <size>' \n",
             "    then the speficied target is assigned the specified request size\n",
             0,0}},
    {"roundrobin",  "rr",
            xddfunc_roundrobin, 
            1,  
            "  -roundrobin # or 'all'\n",  
            {"    Specifies that the threads for multiple targets should be distributed across # processors\n",
             "    If the word 'all' is specified then all available processors will be used.\n",
             0,0,0}},
    {"runtime", "rt",
            xddfunc_runtime,    
            1,  
            "  -runtime #seconds\n",   
            {"    Specifies the number of seconds the entire run should take.\n",
             "    This will set the pass count to infinite and will cause the run to end after the specified number of seconds.\n",
             0,0,0}},
    {"rwratio", "rw",
            xddfunc_rwratio, 
            1,  
            "  -rwratio [target <target#>] <ratio>\n",  
            {"     Specifies the percentage of read operations to write operations.\n",
             "     A value of 0 will result in no read operations and 100% write operations.\n",
             "     A value of 100 will result in 100% read operations and no write operations.\n",
             "     Values between 0 and 100 will adjust the read and write operations accordingly.\n",
             "     Values less than 0 or greater than 100 will result in an error.\n"}},
    {"seek",  "s",
            xddfunc_seek,       
            1,  
            "  -seek [target <target#>] save <filename> | load <filename> | disthist #buckets | seekhist #buckets | sequential | random | range #blocks | stagger | interleave #blocks | seed # | none\n",  
            {"    -seek 'save <filename>' will save the seek list in the file specified\n\
    -seek 'load <filename>' will load the seek list from the file specified\n\
    -seek 'disthist #buckets' will display a 'seek distance' histogram using the specified number of 'buckets'\n\
    -seek 'seekhist #buckets' will display a 'seek location' histogram using the specified number of 'buckets'\n\
    -seek 'sequential' will generate sequential seeks - this is the default \n",
             "    -seek 'random' will generate random seeks over the range specified in -range \n\
    -seek 'range #' is the range of blocksized-blocks over which to operate\n\
    -seek 'stagger' specifies a staggered sequential access over 'range'\n\
    -seek 'interleave #' specifies the number of blocksized blocks to interleave into the access pattern\n\
    -seek 'seed #' specifies a seed to use when generating random numbers\n\
    -seek 'none' do not seek - retransfer the same block each time \n",
                0,0,0}},
    {"setup", "setup",
            xddfunc_setup,      
            1,  
            "  -setup filename\n",  
            {"    Specifies a file contains more command-line options\n", 
            0,0,0,0}},
    {"scsigeneric", "sgio",
            xddfunc_sgio,      
            1,  
            "  -sgio\n",  
            {"    Will use SCSI Generic I/O <linux only>\n", 
            0,0,0,0}},
    {"sharedmemory","shm",
            xddfunc_sharedmemory,
            1,  
            "  -sharedmemory [target <target#>]\n",   
            {"    Will use a shared memory segment instead of the normal malloc/valloc for the I/O buffer.\n", 
            0,0,0,0}},
    {"singleproc",  "sp",
            xddfunc_singleproc, 
            1,  
            "  -singleproc #\n",  
            {"    Specifies that all xdd processes for multiple targets should run on processor #\n",
             "    Requires the processor number to run on\n",
             0,0,0}},
    {"startdelay", "sd",
            xddfunc_startdelay, 
            1,  
            "  -startdelay [target <target#>]#.#seconds\n",   
            {"    Specifies the number of seconds to delay before starting.\n", 
             "    If the startdelay is specified in the format '-startdelay target <targetnumber> <seconds>'",
             "    then the speficied target is assigned the specified start delay\n",
            0,0}},
    {"startoffset", "so",
            xddfunc_startoffset,
            1,  
            "  -startoffset [target <target#>] #\n",  
            {"    Specifies the disk offset in 'blocksize'-byte blocks to begin the operation.\n", 
            0,0,0,0}},
    {"starttime", "stime",
            xddfunc_starttime,  
            1,  
            "  -starttime #seconds\n",  
            {"    Specifies the global start time in seconds - to be used for distributed, synchronize runs \n", 
            0,0,0,0}},
    {"starttrigger","st",
            xddfunc_starttrigger,
            1, 
            "  -starttrigger <target#> <target#> <<time|op|percent|mbytes|kbytes> #>\n",   
            {" ", 0,0,0,0}},
    {"stoptrigger", "et",
            xddfunc_stoptrigger,
            1,  
            "  -stoptrigger <target#> <target#> <<time|op|percent|mbytes|kbytes> #>\n",   
            {" ", 0,0,0,0}},
    {"syncio", "sio",
            xddfunc_syncio,     
            1,  
            "  -syncio #\n",   
            {"    Will synchonize every #th I/O operation.\n", 
            0,0,0,0}},
    {"syncwrite", "sw",
            xddfunc_syncwrite,  
            1,  
            "  -syncwrite [target <target#>]\n",   
            {"    Will cause all write buffers to flush to disk.\n", 
            0,0,0,0}},
    {"target", "target",
            xddfunc_target,    
            1,  
            "  -target filename\n",  
            {"    Specifies the device or file to perform operation on\n",
             "    Required parameter - no default unless -targets option used\n",
             "    See also: -targets\n",
             0,0}},
    {"targetdir",   "td",
            xddfunc_targetdir,  
            1,  
            "  -targetdir [target <target#>] <directory_name>\n",  
            {"    Specifies the target directory that specific target or all targets live in\n", 
            0,0,0,0}},
    {"targetoffset","to",
            xddfunc_targetoffset,
            1,  
            "  -targetoffset #",  
            {"    Specify an offset in 'blocksize'-byte blocks that is multiplied by the target number and added to each target's starting location\n", 
            0,0,0,0}},
    {"targets", "targets",
            xddfunc_targets,    
            1,  
			"  -targets # filename filename filename... -or- -targets -# filename\n",  
            {"    Specifies the devices or files to perform operation on\n",
             "    Requires number of files plus list of devices and/or file names to use as targets \n",
			 "    If the number of devices is negative, then duplicate the single device and/or file name # times\n",
             0,0}},
    {"targetstartdelay", "tsd",
            xddfunc_targetstartdelay, 
            1,  
            "  -targetstartdelay #.#seconds\n",  
            {"    Specify a delay in seconds that is multiplied by the target number and added to each target's start time\n",
            0,0,0,0}},
    {"throttle", "throt",
            xddfunc_throttle,   
            1,  
            "  -throttle [target <target#>] <ops|bw|var> <#.#ops | #.#MB/sec | #.#var>\n",   
            {"    -throttle <ops|bw|var> #.# will cause each target to run at the IOPS or bandwidth specified as #.#\n",
             "    -throttle target N ops #.# will cause the target number N to run at the number of ops per second specified as #.#\n",
             "    -throttle target N bw #.#  will cause the target number N to run at the bandwidth specified as #.#\n",
             "    -throttle target N var #.#  specifies that the BW or IOPS rate should vary by the amount specified.\n",
             0}},
    {"timelimit", "tl",
            xddfunc_timelimit,  
            1,  
            "  -timelimit [target <target#>] <#seconds>\n",   
            {"    Specifies the maximum number of seconds for each pass\n", 
            0,0,0,0}},
    {"timerinfo", "ti",
            xddfunc_timerinfo,  
            1,  
            "  -timerinfo\n",  
            {"    Will print out overhead information for various timer functions used\n", 
            0,0,0,0}},
    {"timeserver", "tsvr",
            xddfunc_timeserver, 
            1,  
            "  -timeserver <host hostname | port # | bounce #>\n",  
            {"    -timeserver 'host' will use [hostname] as the time server in a distributed, synchronized run\n", 
             "    -timeserver 'port' will use # as the port number to use to talk to the time server host\n",
             "    -timeserver 'bouce' will use # as the bounce count to use to talk to ping the time server host\n",
             0,0}},
    {"timestamps", "ts",
            xddfunc_timestamp,  
            1,  
            "  -ts [target <target#>] summary|detailed|wrap|oneshot|size #|append|output <filename>|dump <filename>|triggertime <seconds>|triggerop <op#>\n",   
            {"    -ts  'summary' will turn on time stamping with summary reporting option\n\
    -ts  'detailed'  will turn on time stamping with detailed reporting option\n\
    -ts  'wrap'  will cause the timestamp buffer to wrap after N timestamp entries are used. Should be used in conjunction with -ts size.\n\
    -ts  'oneshot' will stop time stamping after all timestamp entries are used.\n\
    -ts  'size #'  will restrict the size of the timestamp buffer to # entries.\n",
             "    -ts  'triggertime #seconds'  will restrict the size of the timestamp buffer to # entries.\n\
    -ts  'triggerop op#'  will restrict the size of the timestamp buffer to # entries.\n\
    -ts  'append'  will append output to existing output file.\n\
    -ts  'output filename' will print the output to file 'filename'. Default output is stdout\n\
    -ts  'dump filename'  will turn on time stamping and dump a binary time stamp file to 'filename'\n\
    Default is no time stamping.\n",
              0,0,0}},
    {"verbose", "verbose",
            xddfunc_verbose,    
            1,  
            "  -verbose\n",  
            {"    Will print out statistics on each pass \n", 
            0,0,0,0}},
    {"verify", "verify",
            xddfunc_verify,     
            1,  
            "  -verify [target <target#>] location|contents\n",   
            {"    -verify  'location'  will verify the block location is correct\n",
             "    -verify  'contents' will verify the contents of the data buffer read is the same as the specified data pattern\n",
             0,0,0}},
    {"version", "ver",
            xddfunc_version,     
            1,  
            "  -version\n",   
            {"    Will print out the version number of this program\n",
             0,0,0,0}},
    {0,0,0,0,0,{0,0,0,0,0}}
};


/*----------------------------------------------------------------------------*/
/* xdd_parse_args() - Parameter parsing - called by xdd_parse() which performed
 * a whole bunch of initialization processing.
 */
void
xdd_parse_args(int32_t argc, char *argv[])
{
    int funci;      // Index into xdd_func[]
    int argi;       // Index into argv
    int arg_count;  // Number of args left to look at
    int status;     // Status from the option-specific subroutine
    int not_found;  // Indicates that an option was not found
    int invalid;    // Indicates an invalid option from one of the xddfunc routines
    char **argvp;

    arg_count=argc-1;
    argi = 1; // argv[0] is the program name so we start at argv[1]
    while (arg_count) {
        while (*(argv[argi]) != '-') {
                argi++;
                if (argi >= argc) {
                    fprintf(stderr,"xdd_parse_args: ignoring extraneous command line arguments\n");
                    arg_count = 0;
                    break;
                }
        } 
        if (arg_count == 0) break;

        funci = 0;
        not_found = 1;
        invalid = 0;
        while (xdd_func[funci].func_name) {
            if ((strcmp(xdd_func[funci].func_name, (char *)((argv[argi])+1)) == 0) || 
                (strcmp(xdd_func[funci].func_alt, (char *)((argv[argi])+1)) == 0)) {
                argvp = &(argv[argi]);
                status = (int)xdd_func[funci].func_ptr(arg_count, argvp);
                if (status == 0) {
                    invalid = 1;
                    break;
                } else if (status == -1) exit(1);
                argi += status;
                arg_count -= status;
                not_found = 0;
                break;
            }
            funci++;
        }
        // Check to see if things worked...
        if (invalid)
            exit(0);

        if (not_found) {
            xddfunc_invalid_option(argi+1, &(argv[argi]));
            exit(0);
        }      
    }
}
/* All the function sub routines ---------------------------------------------*/
/*----------------------------------------------------------------------------*/
int
xddfunc_align(int32_t argc, char *argv[])
{ 
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->align = atoi(argv[args+1]);
		return(args+2);
	} else { /* Make all the alignment values the same */
		xgp->ptds[0].align = atoi(argv[1]);
		for (j=1; j<MAX_TARGETS; j++)
			xgp->ptds[j].align = xgp->ptds[0].align;
		return(2);
	}
} 
/*----------------------------------------------------------------------------*/
int
xddfunc_blocksize(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->block_size = atoi(argv[args+1]);
		if (p->block_size <= 0) {
			fprintf(xgp->errout,"%s: blocksize of %d is not valid. blocksize must be a number greater than 0\n",xgp->progname,p->block_size);
			return(0);
		}
        return(args+2);
	} else { /* set the block size for all targets to the specified operation */
		xgp->ptds[0].block_size = atoi(argv[1]);
		if (xgp->ptds[0].block_size <= 0) {
			fprintf(xgp->errout,"%s: blocksize of %d is not valid. blocksize must be a number greater than 0\n",xgp->progname,xgp->ptds[0].block_size);
			return(0);
		}
		for (j=1; j<MAX_TARGETS; j++) 
			xgp->ptds[j].block_size = xgp->ptds[0].block_size;
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_combinedout(int32_t argc, char *argv[])
{
	xgp->combined_output_filename = argv[1];
	xgp->combined_output = fopen(xgp->combined_output_filename,"a");
	if (xgp->combined_output == NULL) {
		fprintf(stderr,"%s: Error: Cannot open Combined output file %s\n", xgp->progname,argv[1]);
		xgp->combined_output_filename = "";
	} else xgp->global_options |= RX_COMBINED;
    return(2);
}
/*----------------------------------------------------------------------------*/
// Create new target files for each pass.
int
xddfunc_createnewfiles(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
    if (args < 0) {
        fprintf(xgp->errout, "%s: ERROR processing option %s\n",xgp->progname, argv[0]);
        return(0);
    }
	xgp->global_options |= RX_CREATE_NEW_FILES;
	if (target_number >= 0) { /* Set this option value for a specific target */
	    p = &xgp->ptds[target_number];
	    p->target_options |= RX_CREATE_NEW_FILES;
        return(args+1);
    } else { /* Set option for all targets */
	    for (j=0; j<MAX_TARGETS; j++)
		    xgp->ptds[j].target_options |= RX_CREATE_NEW_FILES;
        return(1);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_csvout(int32_t argc, char *argv[])
{
	xgp->csvoutput_filename = argv[1];
	xgp->csvoutput = fopen(xgp->csvoutput_filename,"w");
	if (xgp->csvoutput == NULL) {
		fprintf(stderr,"%s: Error: Cannot open Comma Separated Values output file %s\n", xgp->progname,argv[1]);
		xgp->csvoutput_filename = "";
	} else xgp->global_options |= RX_CSV;
    return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks between passes
// Arguments: -datapattern [target #] # <option> 
// 
int
xddfunc_datapattern(int32_t argc, char *argv[])
{
    int     j;
    int     args;
    int     target_number;
    ptds_t  *p;
    char    *pattern_type; // The pattern type of ascii, hex, random, ...etc
	unsigned char *pattern; // The actual data pattern specified 
	int     pattern_length; // The length of the pattern string from the command line
    int     retval;
  
    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
        pattern_type = (char *)argv[args+1];
        retval = args+2;
    } else {
        p = 0;
        args = 0;
        pattern_type = (char *)argv[1];
        retval = 2;
    }
	if (strcmp(pattern_type, "random") == 0) {  /* make it random data  */
		if (p)  /* set option for the specific target */
            p->target_options |= RX_RANDOM_PATTERN;
		else for (j=0; j<MAX_TARGETS; j++)  // set for all targets
				xgp->ptds[j].target_options |= RX_RANDOM_PATTERN;
	} else if (strcmp(pattern_type, "ascii") == 0) {
		retval++;
		if (p) { /* set option for specific target */
			p->target_options |= RX_ASCII_PATTERN;
			p->data_pattern = (unsigned char *)argv[args+2];
			p->data_pattern_length = strlen((char *)p->data_pattern);
		}
		else for (j=0; j<MAX_TARGETS; j++) {
				xgp->ptds[j].target_options |= RX_ASCII_PATTERN;
				xgp->ptds[j].data_pattern = (unsigned char *)argv[args+2];
				xgp->ptds[j].data_pattern_length = strlen((char *)xgp->ptds[j].data_pattern);
		}
	} else if (strcmp(pattern_type, "hex") == 0) {
		retval++;
		pattern = (unsigned char *)argv[args+2];
	    pattern_length = strlen((char *)pattern);
		if (pattern_length <= 0) {
			fprintf(xgp->errout, "%s: WARNING: 0-length data pattern specified - using 0x00 instead.\n",xgp->progname);
			pattern = "00";
			pattern_length = 2;
		}
		
		// At this point "pattern" points to the ascii string of hex characters (0-9,a-f)
		// and pattern_length is the length of that string divided by 2
	    // Now we call  xdd_atohex() to convert the ascii sting to a hex bunch of digits
        pattern_length = xdd_atohex(pattern, pattern);
		if (p) { /* set option for specific target */
			p->target_options |= RX_HEX_PATTERN;
			p->data_pattern = pattern;
			p->data_pattern_length = pattern_length;
		}
		else for (j=0; j<MAX_TARGETS; j++) {
				xgp->ptds[j].target_options |= RX_HEX_PATTERN;
				xgp->ptds[j].data_pattern = pattern;
				xgp->ptds[j].data_pattern_length = pattern_length;
		}
	} else if (strcmp(pattern_type, "file") == 0) {
		retval++;
		if (p) {/* set option for specific target */
			p->target_options |= RX_FILE_PATTERN;
			p->data_pattern_filename = (char *)argv[args+2];
		} else { 
			for (j=0; j<MAX_TARGETS; j++) {
				xgp->ptds[j].target_options |= RX_FILE_PATTERN;
			    xgp->ptds[j].data_pattern_filename = (char *)argv[args+2];
			}
		}
	} else if (strcmp(pattern_type, "sequenced") == 0) {
		if (p) /* set option for specific target */
			p->target_options |= RX_SEQUENCED_PATTERN;
		else for (j=0; j<MAX_TARGETS; j++) 
				xgp->ptds[j].target_options |= RX_SEQUENCED_PATTERN;
	} else if (strcmp(pattern_type, "replicate") == 0) {
		if (p) /* set option for specific target */
			p->target_options |= RX_REPLICATE_PATTERN;
		else for (j=0; j<MAX_TARGETS; j++) 
				xgp->ptds[j].target_options |= RX_REPLICATE_PATTERN;
	} else {
		if (p) { /* set option for a specific target */ 
			p->target_options |= RX_SINGLECHAR_PATTERN;
			p->data_pattern = (unsigned char *)pattern_type;
		} else {
			for (j=0; j<MAX_TARGETS; j++) { /* set option for all targets */
				xgp->ptds[j].target_options |= RX_SINGLECHAR_PATTERN;
				xgp->ptds[j].data_pattern = (unsigned char *)pattern_type;
			}
		}
    }
    return(retval);
}
/*----------------------------------------------------------------------------*/
// Seconds to delay between passes
int
xddfunc_delay(int32_t argc, char *argv[])
{ 
	xgp->passdelay = atoi(argv[1]);
    return(1);
}
/*----------------------------------------------------------------------------*/
// Delete the target file when complete
int
xddfunc_deletefile(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
    if (args < 0) {
        fprintf(xgp->errout, "%s: ERROR processing option %s\n",xgp->progname, argv[0]);
        return(0);
    }
	if (target_number >= 0) { /* Set this option value for a specific target */
	    p = &xgp->ptds[target_number];
	    p->target_options |= RX_DELETEFILE;
        return(args+1);
    } else { /* Set option for all targets */
	    for (j=0; j<MAX_TARGETS; j++)
		    xgp->ptds[j].target_options |= RX_DELETEFILE;
        return(1);
	}
}
/*----------------------------------------------------------------------------*/
// Deskew the data rates after a run
int
xddfunc_deskew(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_DESKEW;
	xgp->deskew_total_bytes = 0;
	xgp->deskew_total_time = 0;
	xgp->deskew_total_rates = 0;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Arguments: -devicefile [target #]
int
xddfunc_devicefile(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
    if (args < 0) {
        fprintf(xgp->errout, "%s: ERROR processing option %s\n",xgp->progname, argv[0]);
        return(0);
    }
    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Request size for a specific target */
		p = &xgp->ptds[target_number];
		p->target_options |= RX_DEVICEFILE;
        return(args+1);
    } else { /* Set option for all targets */
		for (j=0; j<MAX_TARGETS; j++)
			xgp->ptds[j].target_options |= RX_DEVICEFILE;
        return(1);
	}
} 
/*----------------------------------------------------------------------------*/
// Specify the use of direct I/O for a single target or for all targets
// Arguments: -dio [target #]
int
xddfunc_dio(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;


    args = xdd_parse_target_number(argc, &argv[0], &target_number);
    if (args < 0) {
        fprintf(xgp->errout, "%s: ERROR processing option %s\n",xgp->progname, argv[0]);
        return(0);
    }
    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Request size for a specific target */
	    p = &xgp->ptds[target_number];
		p->target_options |= RX_DIO;
        return(args+1);
    } else {
        /* Set option for all targets */
	    for (j=0; j<MAX_TARGETS; j++)
	        xgp->ptds[j].target_options |= RX_DIO;
        return(1);
    }
}
/*----------------------------------------------------------------------------*/
int
xddfunc_errout(int32_t argc, char *argv[])
{
	xgp->errout_filename = argv[1];
	xgp->errout = fopen(xgp->errout_filename,"w");
	if (xgp->errout == NULL) {
		fprintf(stderr,"%s: Error: Cannot open error output file %s\n", xgp->progname,argv[1]);
		xgp->errout = stderr;
		xgp->errout_filename = "stderr";
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_fullhelp(int32_t argc, char *argv[])
{
    xdd_usage(1);
    return(-1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_heartbeat(int32_t argc, char *argv[])
{
	xgp->heartbeat = atoi(argv[1]);
    return(2);
}
/*----------------------------------------------------------------------------*/
// Specify an identification string for this run
// Arguments: -id commandline|"string"
int
xddfunc_id(int32_t argc, char *argv[])
{
	int32_t	j;
	if (xgp->id_firsttime == 1) { /* zero out the ID string if this is the first time thru */
		*xgp->id = 0;
		xgp->id_firsttime = 0;
	}

	if (strcmp(argv[1], "commandline") == 0) { /* put in the command line */
		for (j=0; j<xgp->argc; j++) {
			strcat(xgp->id,xgp->argv[j]);
			strcat(xgp->id," ");
		}
	} else {
		strcat(xgp->id,argv[1]);
		strcat(xgp->id," ");
	}
	return(2);	
}
/*----------------------------------------------------------------------------*/
// Specify the number of KBytes to transfer per pass (1K=1024 bytes)
// Arguments: -kbytes [target #] #
// 
int
xddfunc_kbytes(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->kbytes = atoi(argv[args+1]);
		p->mbytes = 0;
        return(args+2);
	} else { /* Make all the megabytes the same */
		xgp->ptds[0].kbytes = atoi(argv[1]);
		xgp->ptds[0].mbytes = 0;
		for (j=1; j<MAX_TARGETS; j++){
			xgp->ptds[j].kbytes = xgp->ptds[0].kbytes;
			xgp->ptds[j].mbytes = 0;
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
/*  -lockstep
	-ls
	-lockstepoverlapped
    -lso
* The lockstep and lockstepoverlapped options all follow the same format:  
*   -lockstep <master_target> <slave_target> <when> <howlong> <what> <howmuch> <startup> <completion>
*     0              1              2           3       4        5       6        7           8
* Where "master_target" is the target that tells the slave when to do something.
*   "slave_target" is the target that responds to requests from the master.
*   "when" specifies when the master should tell the slave to do something.
*    The word "when" should be replaced with the word: 
*     "time" 
*     "op"
*     "percent"
*     "mbytes"
*     "kbytes" 
*   "howlong" is either the number of seconds, number of operations, ...etc.
*       - the interval time in seconds (a floating point number) between task requests from the
*      master to the slave. i.e. if this number were 2.3 then the master would request
*      the slave to perform its task every 2.3 seconds until.
*       - the operation number that defines the interval on which the master will request
*      the slave to perform its task. i.e. if the operation number is set to 8 then upon
*      completion of every 8 (master) operations, the master will request the slave to perform
*      its task.
*       - The percentage of operations that must be completed by the master before requesting
*      the slave to perform a task.
*       - the number of megabytes (1024*1024 bytes) or the number of kilobytes (1024 bytes)
*    "what" is the type of task the slave should perform each time it is requested to perform
*     a task by the master. The word "what" should be replaced by:
*     "time" 
*     "op"
*     "mbytes"
*     "kbytes" 
*   "howmuch" is either the number of seconds, number of operations, ...etc.
*       - the amount of time in seconds (a floating point number) the slave should run before
*      pausing and waiting for further requests from the master.
*       - the number of operations the slave should perform before pausing and waiting for
*      further requests from the master.
*       - the number of megabytes (1024*1024 bytes) or the number of kilobytes (1024 bytes)
*      the slave should transfer before pausing and waiting for
*      further requests from the master.
*    "startup" is either "wait" or "run" depending on whether the slave should start running upon
*      invocation and perform a single task or if it should simply wait for the master to 
*      request it to perform its first task. 
*    "Completion" - in the event that the master finishes before the slave, then the slave will have
*      the option to complete all of its remaining operations or to just stop at this point. 
*      This should be specified as either "complete" or "stop". I suppose more sophisticated
*      behaviours could be defined but this is it for now.
*       
*/
xddfunc_lockstep(int32_t argc, char *argv[])
{
    int mt,st;
    double tmpf;
    ptds_t *masterp, *slavep;
    int retval;
    int lsmode;
    char *when, *what, *lockstep_startup, *lockstep_completion;

	if ((strcmp(argv[0], "-lockstep") == 0) ||
		(strcmp(argv[0], "-ls") == 0))
		lsmode = RX_LOCKSTEP;
	else lsmode = RX_LOCKSTEPOVERLAPPED;

	mt = atoi(argv[1]); /* T1 is the master target */
	st = atoi(argv[2]); /* T2 is the slave target */
	/* Sanity checks on the target numbers */
	masterp = &xgp->ptds[mt]; 
	slavep = &xgp->ptds[st];
	/* Both the master and the slave must know that they are in lockstep. */
	masterp->target_options |= lsmode;
	slavep->target_options |= lsmode; 
	masterp->ls_slave = st; /* The master has to know the number of the slave target */
	slavep->ls_master = mt; /* The slave has to know the target number of its master */
	when = argv[3];

	if (strcmp(when,"time") == 0){ /* get the number of seconds to wait before triggering the other target */
		tmpf = atof(argv[4]);
		masterp->ls_interval_type = LS_INTERVAL_TIME;
		masterp->ls_interval_units = "SECONDS";
		masterp->ls_interval_value = (pclk_t)(tmpf * TRILLION);
		if (masterp->ls_interval_value <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep interval time: %f. This value must be greater than 0.0\n",
				xgp->progname, tmpf);
            return(0);
		};
		retval = 5;
	} else if (strcmp(when,"op") == 0){ /* get the number of operations to wait before triggering the other target */
		masterp->ls_interval_value = atoll(argv[4]);
		masterp->ls_interval_type = LS_INTERVAL_OP;
		masterp->ls_interval_units = "OPERATIONS";
		if (masterp->ls_interval_value <= 0) {
#ifdef WIN32
			fprintf(stderr,"%s: Invalid lockstep interval op: %I64d. This value must be greater than 0\n",
#else
			fprintf(stderr,"%s: Invalid lockstep interval op: %lld. This value must be greater than 0\n",
#endif
				xgp->progname, masterp->ls_interval_value);
            return(0);
		}
		retval = 5;  
	} else if (strcmp(when,"percent") == 0){ /* get the percentage of operations to wait before triggering the other target */
		masterp->ls_interval_value = (uint64_t)(atof(argv[4]) / 100.0);
		masterp->ls_interval_type = LS_INTERVAL_PERCENT;
		masterp->ls_interval_units = "PERCENT";
		if ((masterp->ls_interval_value < 0.0) || (masterp->ls_interval_value > 1.0)) {
			fprintf(stderr,"%s: Invalid lockstep interval percent: %f. This value must be between 0.0 and 100.0\n",
				xgp->progname, atof(argv[4]) );
            return(0);
		}
		retval = 5;    
	} else if (strcmp(when,"mbytes") == 0){ /* get the number of megabytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		masterp->ls_interval_value = (uint64_t)(tmpf * 1024*1024);
		masterp->ls_interval_type = LS_INTERVAL_BYTES;
		masterp->ls_interval_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep interval mbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		retval = 5;    
	} else if (strcmp(when,"kbytes") == 0){ /* get the number of kilobytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		masterp->ls_interval_value = (uint64_t)(tmpf * 1024);
		masterp->ls_interval_type = LS_INTERVAL_BYTES;
		masterp->ls_interval_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep interval kbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		retval = 5;   
	} else {
		fprintf(stderr,"%s: Invalid lockstep interval qualifer: %s\n",
				xgp->progname, when);
        return(0);
	}
	/* This section looks at what the slave target is supposed to do for a "task" */
	what = argv[5];
	if (strcmp(what,"time") == 0){ /* get the number of seconds to run a task */
		tmpf = atof(argv[6]);
		slavep->ls_task_type = LS_TASK_TIME;
		slavep->ls_task_units = "SECONDS";
		slavep->ls_task_value = (pclk_t)(tmpf * TRILLION);
		if (slavep->ls_task_value <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep task time: %f. This value must be greater than 0.0\n",
				xgp->progname, tmpf);
            return(0);
		};
		retval += 2;
	} else if (strcmp(what,"op") == 0){ /* get the number of operations to execute per task */
		slavep->ls_task_value = atoll(argv[6]);
		slavep->ls_task_type = LS_TASK_OP;
		slavep->ls_task_units = "OPERATIONS";
		if (slavep->ls_task_value <= 0) {
#ifdef WIN32
			fprintf(stderr,"%s: Invalid lockstep task op: %I64d. This value must be greater than 0\n",
#else
			fprintf(stderr,"%s: Invalid lockstep task op: %lld. This value must be greater than 0\n",
#endif
				xgp->progname, slavep->ls_task_value);
            return(0);
		}
		retval += 2;       
	} else if (strcmp(what,"mbytes") == 0){ /* get the number of megabytes to transfer per task */
		tmpf = atof(argv[6]);
		slavep->ls_task_value = (uint64_t)(tmpf * 1024*1024);
		slavep->ls_task_type = LS_TASK_BYTES;
		slavep->ls_task_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep task mbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
        return(0);
		}
		retval += 2;     
	} else if (strcmp(what,"kbytes") == 0){ /* get the number of kilobytes to transfer per task */
		tmpf = atof(argv[6]);
		slavep->ls_task_value = (uint64_t)(tmpf * 1024);
		slavep->ls_task_type = LS_TASK_BYTES;
		slavep->ls_task_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep task kbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
        return(0);
		}
		retval += 2;    
	} else {
		fprintf(stderr,"%s: Invalid lockstep task qualifer: %s\n",
				xgp->progname, what);
        return(0);
	}
	lockstep_startup = argv[7];  
	if (strcmp(lockstep_startup,"run") == 0) { /* have the slave start running immediately */
		slavep->ls_ms_state |= LS_SLAVE_RUN_IMMEDIATELY;
		slavep->ls_ms_state &= ~LS_SLAVE_WAITING;
		slavep->ls_task_counter = 1;
	} else { /* Have the slave wait for the master to tell it to run */
		slavep->ls_ms_state &= ~LS_SLAVE_RUN_IMMEDIATELY;
		slavep->ls_ms_state |= LS_SLAVE_WAITING;
		slavep->ls_task_counter = 0;
	}
    retval++;
	lockstep_completion = argv[8];
	if (strcmp(lockstep_completion,"complete") == 0) { /* Have slave complete all operations if master finishes first */
		slavep->ls_ms_state |= LS_SLAVE_COMPLETE;
	} else if (strcmp(lockstep_completion,"stop") == 0){ /* Have slave stop when master stops */
		slavep->ls_ms_state |= LS_SLAVE_STOP;
	} else {
		fprintf(stderr,"%s: Invalid lockstep slave completion directive: %s. This value must be either 'complete' or 'stop'\n",
				xgp->progname,lockstep_completion);
        return(0);
    }
    retval++;

    return(retval);
 }
/*----------------------------------------------------------------------------*/
// Set the maxpri and plock
int
xddfunc_maxall(int32_t argc, char *argv[])
{
    int status;

    status = xddfunc_maxpri(1,0);
    status += xddfunc_plock(1,0);
    if (status < 2)
        return(0);
    else return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the maximum number of errors to tolerate before exiting
int
xddfunc_maxerrors(int32_t argc, char *argv[])
{
	xgp->max_errors = atoll(argv[1]);
    return(1);
}
/*----------------------------------------------------------------------------*/
// Set the maximum runtime priority
int
xddfunc_maxpri(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_MAXPRI;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the number of MBytes to transfer per pass (1M=1024*1024 bytes)
// Arguments: -mbytes [target #] #
// 
int
xddfunc_mbytes(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->mbytes = atoi(argv[args+1]);
		p->kbytes = 0;
        return(args+2);
	} else { /* Make all the megabytes the same */
		xgp->ptds[0].mbytes = atoi(argv[1]);
		xgp->ptds[0].kbytes = 0;
		for (j=1; j<MAX_TARGETS; j++) {
			xgp->ptds[j].mbytes = xgp->ptds[0].mbytes;
			xgp->ptds[j].kbytes = 0;
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Set the  no mem lock and no proc lock flags 
int
xddfunc_minall(int32_t argc, char *argv[])
{
    int status;

    status = xddfunc_nomemlock(1,0);
    status += xddfunc_noproclock(1,0);
    if (status < 2)
        return(0);
    else return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_nobarrier(int32_t argc, char *argv[])
{
    xgp->global_options |= RX_NOBARRIER;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Set the no memory lock flag
int
xddfunc_nomemlock(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_NOMEMLOCK;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Set the no process lock flag
int
xddfunc_noproclock(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_NOPROCLOCK;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the number of requests to run 
// Arguments: -numreqs [target #] #
// 
int
xddfunc_numreqs(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->numreqs = atoll(argv[args+1]);
		if (p->numreqs <= 0) {
#ifdef WIN32
			fprintf(xgp->errout,"%s: numreqs of %I64d is not valid. numreqs must be a number greater than 0\n",xgp->progname,p->numreqs);
#else
			fprintf(xgp->errout,"%s: numreqs of %lld is not valid. numreqs must be a number greater than 0\n",xgp->progname,p->numreqs);
#endif
            return(0);
		}
        return(args+2);
	} else { /* Make all the "number of requests" the same */
		xgp->ptds[0].numreqs = atoll(argv[1]);
		if (xgp->ptds[0].numreqs <= 0) {
#ifdef WIN32
			fprintf(xgp->errout,"%s: numreqs of %I64d is not valid. numreqs must be a number greater than 0\n",xgp->progname,xgp->ptds[0].numreqs);
#else
			fprintf(xgp->errout,"%s: numreqs of %lld is not valid. numreqs must be a number greater than 0\n",xgp->progname,xgp->ptds[0].numreqs);
#endif
			return(0);
		}
		for (j=1; j<MAX_TARGETS; j++)
			xgp->ptds[j].numreqs = xgp->ptds[0].numreqs;
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the operation to perform - read or write 
// Arguments: -op [target #] read|write
int
xddfunc_operation(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		if (strcmp(argv[args+1], "write") == 0)
			p->rwratio = 0.0; /* all write operations for this target */
		else p->rwratio = 1.0; /* all read operations for this target */
        return(args+2);
	} else { /* set the operations for all targets to the specified operation */
		p = &xgp->ptds[0];
		if (strcmp(argv[1], "write") == 0)
			p->rwratio = 0.0; /* all write operations  */
		else p->rwratio = 1.0; /* all read operations  */

		for (j=1; j<MAX_TARGETS; j++) 
		    xgp->ptds[j].rwratio = p->rwratio;
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_output(int32_t argc, char *argv[])
{
	xgp->output_filename = argv[1];
	xgp->output = fopen(xgp->output_filename,"w");
	if (xgp->output == NULL) {
		fprintf(xgp->errout,"%s: Error: Cannot open output file %s\n", xgp->progname,argv[1]);
		xgp->output = stdout;
		xgp->output_filename = "stdout";
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_passes(int32_t argc, char *argv[])
{
    xgp->passes = atoi(argv[1]);
    return(2);
}

/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks between passes
// Arguments: -passoffset [target #] #
// 
int
xddfunc_passoffset(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->pass_offset = atoll(argv[args+1]);
        return(args+2);
	} else { /* Make all the pass offsets the same */
		xgp->ptds[0].pass_offset = atoll(argv[1]);
		for (j=1; j<MAX_TARGETS; j++)
			xgp->ptds[j].pass_offset = xgp->ptds[0].pass_offset;
        return(2);
    } 
}
/*----------------------------------------------------------------------------*/
// Lock the process in memory
int
xddfunc_plock(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_PLOCK;
    return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_preallocate(int32_t argc, char *argv[])
{ 
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->preallocate = atoi(argv[args+1]);
		return(args+2);
	} else { /* Make all the preallocate values the same for all targets */
		xgp->ptds[0].preallocate = atoi(argv[1]);
		for (j=1; j<MAX_TARGETS; j++)
			xgp->ptds[j].preallocate = xgp->ptds[0].preallocate;
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
// processor/target assignment
int
xddfunc_processor(int32_t argc, char *argv[])
{
    int cpus;
    int processor_number;
    int target_number;
    ptds_t *p;

#if (LINUXUP || HPUX || OSX)
	cpus = 1;
	fprintf(xgp->errout,"%s: WARNING: Multiple processors not supported in this release\n",xgp->progname);
#elif (SOLARIS || AIX)
	/* SOLARIS or AIX */ 
	cpus = sysconf(_SC_NPROCESSORS_ONLN);
#elif (IRIX || WIN32)
	/* IRIX */
	cpus = sysmp(MP_NPROCS,0);
#elif (LINUXSMP)
	cpus = xdd_linux_cpu_count();
#endif
	processor_number = atoi(argv[1]); /* processor to run on */
	if ((processor_number < 0) || (processor_number >= cpus)) {
		fprintf(xgp->errout,"%s: Error: Processor number <%d> is out of range\n",xgp->progname, processor_number);
		fprintf(xgp->errout,"%s:     Processor number should be between 0 and %d\n",xgp->progname, cpus-1);
        return(0);
	}
	target_number = atoi(argv[2]); /* target number to run on this processor */
	if (target_number >= MAX_TARGETS) {
		fprintf(xgp->errout,"%s: Error: Target number of %d on processor assignment option is out of range\n",xgp->progname, target_number);
		fprintf(xgp->errout,"%s:     Target number should be between 0 and %d\n",xgp->progname, MAX_TARGETS-1);
        return(0);
	}
	p = &xgp->ptds[target_number];
	p->processor = processor_number;
    return(3);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_queuedepth(int32_t argc, char *argv[])
{ 
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->queue_depth = atoi(argv[args+1]);
        return(args+2);
	} else { /* Make all the queue depths the same */
		xgp->ptds[0].queue_depth = atoi(argv[1]);
		for (j=1; j<MAX_TARGETS; j++)
			xgp->ptds[j].queue_depth = xgp->ptds[0].queue_depth;
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_qthreadinfo(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_QTHREAD_INFO;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Specify that the seek list should be randomized between passes
// Arguments: -randomize [target #]

int
xddfunc_randomize(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;


    args = xdd_parse_target_number(argc, &argv[0], &target_number);
    if (args < 0) {
        fprintf(xgp->errout, "%s: ERROR processing option %s\n",xgp->progname, argv[0]);
        return(0);
    }
    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->target_options |= RX_PASSRANDOMIZE;
        return(args+1);
    }
    /* Set option for all targets */
    for (j=0; j<MAX_TARGETS; j++) 
		xgp->ptds[j].target_options |= RX_PASSRANDOMIZE;
	return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the read-after-write options for either the reader or the writer
// Arguments: -readafterwrite [target #] option_name value
// Valid options are trigger [stat | mp]
//                   lag <#>
//                   reader <hostname>
//                   port <#>
// 
int
xddfunc_readafterwrite(int32_t argc, char *argv[])
{
    int     i;
    int     args;
    int     target_number;
    ptds_t  *p;

	i = 1;
    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		if (target_number >= MAX_TARGETS) { /* Make sure the target number is somewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for read-after-write option %s\n",
					xgp->progname, target_number, argv[i+2]);
			return(0);
		}
		i += args; /* skip past the "target <taget number>" */
	}
	/* At this point "i" points to the raw "option" argument */
	if (strcmp(argv[i], "trigger") == 0) { /* set the the trigger type */
		if (target_number < 0) {  /* set option for all targets */
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {
				xgp->ptds[target_number].target_options |= RX_READAFTERWRITE;
				if (strcmp(argv[i+1], "stat") == 0)
					xgp->ptds[target_number].raw_trigger |= RX_RAW_STAT;
				else if (strcmp(argv[i+1], "mp") == 0)
					xgp->ptds[target_number].raw_trigger |= RX_RAW_MP;
				else {
					fprintf(stderr,"%s: Invalid trigger type specified for read-after-write option: %s\n",
						xgp->progname, argv[i+1]);
					return(0);
				}
			}
		} else {  /* set option for specific target */
			p->target_options |= RX_READAFTERWRITE;
			if (strcmp(argv[i+1], "stat") == 0)
					p->raw_trigger |= RX_RAW_STAT;
				else if (strcmp(argv[i+1], "mp") == 0)
					p->raw_trigger |= RX_RAW_MP;
				else {
					fprintf(stderr,"%s: Invalid trigger type specified for read-after-write option: %s\n",
						xgp->progname, argv[i+1]);
					return(0);
				}
		}
        return(i+2);
	} else if (strcmp(argv[i], "lag") == 0) { /* set the lag block count */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].target_options |= RX_READAFTERWRITE;
				xgp->ptds[target_number].raw_lag = atoi(argv[i+1]);
			}
		} else {  /* set option for specific target */
			p->target_options |= RX_READAFTERWRITE;
			p->raw_lag = atoi(argv[i+1]);
		}
        return(i+2);
	} else if (strcmp(argv[i], "reader") == 0) { /* hostname of the reader for this read-after-write */
		/* This assumes that these targets are all writers and need to know who the reader is */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].target_options |= RX_READAFTERWRITE;
				xgp->ptds[target_number].raw_hostname = argv[i+1];
			}
		} else {  /* set option for specific target */
			p->target_options |= RX_READAFTERWRITE;
			p->raw_hostname = argv[i+1];
		}
        return(i+2);
	} else if (strcmp(argv[i], "port") == 0) { /* set the port number for the socket used by the writer */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].target_options |= RX_READAFTERWRITE;
				xgp->ptds[target_number].raw_port = atoi(argv[i+1]);
			}
		} else {  /* set option for specific target */
			p->target_options |= RX_READAFTERWRITE;
			p->raw_port = atoi(argv[i+1]);
		}
        return(i+2);
	} else {
			fprintf(stderr,"%s: Invalid Read-after-write option %s\n",xgp->progname, argv[i+1]);
            return(0);
    }/* End of the -readafterwrite (raw) sub options */
}
/*----------------------------------------------------------------------------*/
int
xddfunc_reallyverbose(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_REALLYVERBOSE;
    return(1);
}

/*----------------------------------------------------------------------------*/
// Re-create the target file between each pass
int
xddfunc_recreatefiles(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
    if (args < 0) {
        fprintf(xgp->errout, "%s: ERROR processing option %s\n",xgp->progname, argv[0]);
        return(0);
    }
    xgp->global_options |= RX_RECREATE;
	if (target_number >= 0) { /* Set this option value for a specific target */
	    p = &xgp->ptds[target_number];
	    p->target_options |= RX_RECREATE;
        return(args+1);
    } else { /* Set option for all targets */
	    for (j=0; j<MAX_TARGETS; j++)
		    xgp->ptds[j].target_options |= RX_RECREATE;
        return(1);
	}
}

/*----------------------------------------------------------------------------*/
// Re-open the target file between each pass
int
xddfunc_reopen(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
    if (args < 0) {
        fprintf(xgp->errout, "%s: ERROR processing option %s\n",xgp->progname, argv[0]);
        return(0);
    }
    xgp->global_options |= RX_REOPEN;
	if (target_number >= 0) { /* Set this option value for a specific target */
	    p = &xgp->ptds[target_number];
	    p->target_options |= RX_REOPEN;
        return(args+1);
    } else { /* Set option for all targets */
	    for (j=0; j<MAX_TARGETS; j++)
		    xgp->ptds[j].target_options |= RX_REOPEN;
        return(1);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the reporting threshold for I/O operations that take more than a
// certain time to complete for either a single target or all targets 
// Arguments: -reportthreshold [target #] #.#
int
xddfunc_report_threshold(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    double tmpf;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		tmpf = atof(argv[args+1]);
		if (tmpf < 0.0) {
			fprintf(xgp->errout,"%s: report threshold of %5.2f is not valid. rwratio must be a positive number\n",xgp->progname,tmpf);
            return(0);
		}
		p->report_threshold = (pclk_t)(tmpf * TRILLION);
        return(args+2);
	} else { /* set the rw ratios for all targets to the specified operation */
		tmpf = atof(argv[1]);
		if (tmpf < 0.0) {
			fprintf(xgp->errout,"%s: report threshold of %5.2f is not valid. rwratio must be a positive number\n",xgp->progname,tmpf);
            return(0);
		}
		xgp->ptds[0].report_threshold = (pclk_t)(tmpf * TRILLION);
		for (j=1; j<MAX_TARGETS; j++) 
			xgp->ptds[j].report_threshold = xgp->ptds[0].report_threshold;
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the I/O request size in blocks for either a single target or all targets 
// Arguments: -reqsize [target #] #
int
xddfunc_reqsize(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->reqsize = atoi(argv[args+1]);
		if (p->reqsize <= 0) {
			fprintf(xgp->errout,"%s: reqsize of %d is not valid. reqsize must be a number greater than 0\n",xgp->progname,p->reqsize);
			return(0);
		}
        return(args+2);
	} else { /* Make all the request sizes the same */
		xgp->ptds[0].reqsize = atoi(argv[1]);
		if (xgp->ptds[0].reqsize <= 0) {
			fprintf(xgp->errout,"%s: reqsize of %d is not valid. reqsize must be a number greater than 0\n",xgp->progname,xgp->ptds[0].reqsize);
			return(0);
		}
		for (j=1; j<MAX_TARGETS; j++)
			xgp->ptds[j].reqsize = xgp->ptds[0].reqsize;
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// round robin processor target assignment
int
xddfunc_roundrobin(int32_t argc, char *argv[])
{
    int j,k;
    int cpus;
    int processor_number;
    ptds_t *p;

#if (LINUXUP || HPUX || OSX)
	cpus = 1;
	fprintf(xgp->errout,"%s: WARNING: Multiple processors not supported in this release\n",xgp->progname);
#elif (SOLARIS || AIX)
	/* SOLARIS or AIX */ 
	cpus = sysconf(_SC_NPROCESSORS_ONLN);
#elif (IRIX || WIN32)
	/* IRIX */
	cpus = sysmp(MP_NPROCS,0);
#elif (LINUXSMP)
	cpus = xdd_linux_cpu_count();
#endif
	processor_number = atoi(argv[1]);
	if ((processor_number < 1) || (processor_number > cpus)) {
		fprintf(xgp->errout,"%s: Error: Number of processors <%d> is out of range\n",xgp->progname, processor_number);
		fprintf(xgp->errout,"%s:     Number of processors should be between 1 and %d\n",xgp->progname, cpus);
		return(0);
	}
	k = 0;
	for (j = 0; j < MAX_TARGETS; j++) {
		p = &xgp->ptds[j];
		p->processor = k;
		k++;
		if (k >= processor_number)
			k = 0;
	}
    return(argc);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_runtime(int32_t argc, char *argv[])
{
    int j;

	xgp->runtime = atoi(argv[1]);
	for (j=0; j<MAX_TARGETS; j++) 
        xgp->ptds[j].ts_options |= TS_WRAP; /* Turn on time stamp wrapping just in case */
    return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the read/write ratio for either a single target or all targets 
// Arguments: -rwratio [target #] #.#
int
xddfunc_rwratio(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    double tmpf;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		tmpf = (atof(argv[args+1]) / 100.0);
		if ((tmpf < 0.0) || (tmpf > 1.0)) {
			fprintf(xgp->errout,"%s: rwratio of %5.2f is not valid. rwratio must be a number between 0.0 and 100.0\n",xgp->progname,tmpf);
            return(0);
		}
		p->rwratio = tmpf;
        return(args+2);
	} else { /* set the rw ratios for all targets to the specified operation */
		tmpf = (atof(argv[1]) / 100.0);
		if ((tmpf < 0.0) || (tmpf > 1.0)) {
			fprintf(xgp->errout,"%s: rwratio of %5.2f is not valid. rwratio must be a number between 0.0 and 100.0\n",xgp->progname,tmpf);
            return(0);
		}
		xgp->ptds[0].rwratio = tmpf;
		for (j=1; j<MAX_TARGETS; j++) 
			xgp->ptds[j].rwratio = xgp->ptds[0].rwratio;
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks between passes
// Arguments: -seek [target #] option_name value
// 
int
xddfunc_seek(int32_t argc, char *argv[])
{
    int     i;
    int     args;
    int     target_number;
    ptds_t  *p;

	i = 1;
    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		if (target_number >= MAX_TARGETS) { /* Make sure the target number is somewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for seek option %s\n",
					xgp->progname, target_number, argv[i+args]);
            return(0);
		}
		i += args; /* skip past the "target <taget number>" */
	}
	/* At this point "i" points to the seek "option" argument */
	if (strcmp(argv[i], "save") == 0) { /* save the seek information in a file */
		if (target_number < 0) {  /* set option for all targets */
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {
				xgp->ptds[target_number].seekhdr.seek_options |= RX_SEEK_SAVE;
				xgp->ptds[target_number].seekhdr.seek_savefile = argv[i+1];
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_options |= RX_SEEK_SAVE;
			p->seekhdr.seek_savefile = argv[i+1];
		}
		return(i+2);
	} else if (strcmp(argv[i], "load") == 0) { /* load seek list from "filename" */
		if (target_number < 0) {  /* set option for all targets */
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {
				xgp->ptds[target_number].seekhdr.seek_options |= RX_SEEK_LOAD;
				xgp->ptds[target_number].seekhdr.seek_loadfile = argv[i+1];
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_options |= RX_SEEK_LOAD;
			p->seekhdr.seek_loadfile = argv[i+1];
		}
		return(i+2);
	} else if (strcmp(argv[i], "disthist") == 0) { /*  Print a Distance Histogram */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].seekhdr.seek_options |= RX_SEEK_DISTHIST;
				xgp->ptds[target_number].seekhdr.seek_NumDistHistBuckets = atoi(argv[i+1]);
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_options |= RX_SEEK_DISTHIST;
			p->seekhdr.seek_NumDistHistBuckets = atoi(argv[i+1]);
		}
		return(i+2);
	} else if (strcmp(argv[i], "seekhist") == 0) { /* Print a Seek Histogram */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].seekhdr.seek_options |= RX_SEEK_SEEKHIST;
				xgp->ptds[target_number].seekhdr.seek_NumSeekHistBuckets = atoi(argv[i+1]);
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_options |= RX_SEEK_SEEKHIST;
			p->seekhdr.seek_NumSeekHistBuckets = atoi(argv[i+1]);
		}
		return(i+2);
	} else if (strcmp(argv[i], "sequential") == 0) { /*  Sequential seek list option */     
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].seekhdr.seek_options &= ~RX_SEEK_RANDOM;
				xgp->ptds[target_number].seekhdr.seek_pattern = "sequential";
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_options &= ~RX_SEEK_RANDOM;
			p->seekhdr.seek_pattern = "sequential";
		}
		return(i+1);
	} else if (strcmp(argv[i], "random") == 0) { /*  Random seek list option */     
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].seekhdr.seek_options |= RX_SEEK_RANDOM;
				xgp->ptds[target_number].seekhdr.seek_pattern = "random";
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_options |= RX_SEEK_RANDOM;
			p->seekhdr.seek_pattern = "random";
		}
		return(i+1);
	} else if (strcmp(argv[i], "stagger") == 0) { /*  Staggered seek list option */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].seekhdr.seek_options |= RX_SEEK_STAGGER;
				xgp->ptds[target_number].seekhdr.seek_pattern = "staggered";
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_options |= RX_SEEK_STAGGER;
			p->seekhdr.seek_pattern = "staggered";
		}
		return(i+1);
	} else if (strcmp(argv[i], "interleave") == 0) { /* set the interleave for sequential seek locations */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].seekhdr.seek_interleave = atoi(argv[i+1]);
				xgp->ptds[target_number].seekhdr.seek_pattern = "interleaved";
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_interleave = atoi(argv[i+1]);
			p->seekhdr.seek_pattern = "interleaved";
		}
		return(i+2);
	} else if (strcmp(argv[i], "none") == 0) { /* no seeking at all */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].seekhdr.seek_options |= RX_SEEK_NONE;
				xgp->ptds[target_number].seekhdr.seek_pattern = "none";
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_options |= RX_SEEK_NONE;
			p->seekhdr.seek_pattern = "none";
		}
		return(i+1);
	} else if (strcmp(argv[i], "range") == 0) { /* set the range of seek locations */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].seekhdr.seek_range = atoll(argv[i+1]);
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_range = atoll(argv[i+1]);
		}
		return(i+2);
	} else if (strcmp(argv[i], "seed") == 0) { /* set the seed for random seek locations */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
				xgp->ptds[target_number].seekhdr.seek_seed = atoi(argv[i+1]);
			}
		} else {  /* set option for specific target */
			p->seekhdr.seek_seed = atoi(argv[i+1]);
		}
		return(i+2);
    } else {
			fprintf(stderr,"%s: Invalid Seek option %s\n",xgp->progname, argv[i+1]);
            return(0);
    } /* End of the -seek sub options */
}
/*----------------------------------------------------------------------------*/
int
xddfunc_setup(int32_t argc, char *argv[])
{
    int status;
    status = xdd_process_paramfile(argv[1]);
    if (status == 0)
        return(0);
    else return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the use of SCSI Generic I/O for a single target or for all targets
// Arguments: -sgio [target #]
int
xddfunc_sgio(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;


    args = xdd_parse_target_number(argc, &argv[0], &target_number);
    if (args < 0) {
        fprintf(xgp->errout, "%s: ERROR processing option %s\n",xgp->progname, argv[0]);
        return(0);
    }
    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Request size for a specific target */
	    p = &xgp->ptds[target_number];
		p->target_options |= RX_SGIO;
        return(args+1);
    } else {
        /* Set option for all targets */
	    for (j=0; j<MAX_TARGETS; j++)
	        xgp->ptds[j].target_options |= RX_SGIO;
        return(1);
    }
}
/*----------------------------------------------------------------------------*/
int
xddfunc_sharedmemory(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
    if (args < 0) {
        fprintf(xgp->errout, "%s: ERROR processing option %s\n",xgp->progname, argv[0]);
        return(0);
    }
    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Request size for a specific target */
		p = &xgp->ptds[target_number];
		p->target_options |= RX_SHARED_MEMORY;
        return(args+1);
    } 
    /* Make all the alignment values the same */
	for (j=0; j<MAX_TARGETS; j++)
		xgp->ptds[j].target_options |= RX_SHARED_MEMORY;
    return(1);
}
/*----------------------------------------------------------------------------*/	
// single processor scheduling
int
xddfunc_singleproc(int32_t argc, char *argv[]) 
{
    int j;
    int cpus;
    int processor_number;
    ptds_t *p;

#if (LINUXUP || HPUX || OSX)
	cpus = 1;
	fprintf(xgp->errout,"%s: WARNING: Multiple processors not supported in this release\n",xgp->progname);
#elif (SOLARIS || AIX)
	/* SOLARIS or AIX */ 
	cpus = sysconf(_SC_NPROCESSORS_ONLN);
#elif (IRIX || WIN32)
	/* IRIX */
	cpus = sysmp(MP_NPROCS,0);
#elif (LINUXSMP)
	cpus = xdd_linux_cpu_count();
#endif
		processor_number = atoi(argv[1]);
	if ((processor_number < 0) || (processor_number >= cpus)) {
		fprintf(xgp->errout,"%s: Error: Processor number <%d> is out of range\n",xgp->progname, processor_number);
		fprintf(xgp->errout,"%s:     Processor number should be between 0 and %d\n",xgp->progname, cpus-1);
		return(0);
	}
	for (j = 0; j < MAX_TARGETS; j++) {
		p = &xgp->ptds[j];
		p->processor = processor_number;
	}
    return(argc);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_startdelay(int32_t argc, char *argv[])
{ 
    int j;
    int args;
    int target_number;
    double tmpf;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		tmpf = atof(argv[args+1]);
		p->start_delay = (pclk_t)(tmpf * TRILLION);
		return(args+2);
	}
	else { /* Make all the time limits the same */
		tmpf = atof(argv[1]);
		xgp->ptds[0].start_delay = (pclk_t)(tmpf * TRILLION);
		for (j=1; j<MAX_TARGETS; j++)
			xgp->ptds[j].start_delay = xgp->ptds[0].start_delay;
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks
// Arguments: -startoffset [target #] #
// 
int
xddfunc_startoffset(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->start_offset = atoll(argv[args+1]);
        return(args+2);
	} else { /* Make all the starting offsets the same */
		xgp->ptds[0].start_offset = atoll(argv[1]);
		for (j=1; j<MAX_TARGETS; j++)
			xgp->ptds[j].start_offset = xgp->ptds[0].start_offset;
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_starttime(int32_t argc, char *argv[])
{ 
	xgp->gts_time = (pclk_t) atoll(argv[1]);
	xgp->gts_time *= TRILLION;
    return(2);
}
/*----------------------------------------------------------------------------*/
/* The Trigger options all fillow the same format:  
*                -xxxtrigger <target1> <target2> <when #>
                    0           1          2       3   4 
* Where "target1" is the target that generates the trigger and sends it to target2.
*   "target2" is the target that responds to the trigger from trigger 1.
*   "when #" specifies when the trigger should happen. The word "when" should be replaced with
*    the word "time", "op",  "percent", "mbytes", or "kbytes" followed by "howlong" is 
*     either the number of seconds, number of operations, or a percentage from 0.1-100.0, ...etc.
*       - the time in seconds (a floating point number) as measured from the start of the pass
*      before the trigger is sent to target2
*       - the operation number that target1 should reach before sending trigger to target2
*       - the percentage of data that target1 should transfer before the trigger is sent
*      to target2.
*       - the number of megabytes (1024*1024 bytes) or the number of kilobytes (1024 bytes)
*       
* Usage notes:
* Target1 should not equal target2 - this does not make sense.
* Target1 and target2 should not try to single each other to start - this is not possible.
* Target1 can signal target2 to start and target2 can later signal target1 to stop.
*       
*/
int
xddfunc_starttrigger(int32_t argc, char *argv[])
{
    int t1,t2;
    double tmpf;
    ptds_t *p1, *p2;
    char *when;


	t1 = atoi(argv[1]); /* T1 is the target that does the triggering */
	t2 = atoi(argv[2]); /* T2 is the target that gets triggered by T1 */
	/* Sanity checks on the target numbers */
	p1 = &xgp->ptds[t1]; 
	p2 = &xgp->ptds[t2];
	p1->start_trigger_target = t2; /* The target that does the triggering has to 
									* know the target number to trigger */
	p2->target_options |= RX_WAITFORSTART; /* The target that willbe triggered has to know to wait */
	when = argv[3];

	if (strcmp(when,"time") == 0){ /* get the number of seconds to wait before triggering the other target */
		tmpf = atof(argv[4]);
		p1->start_trigger_time = (pclk_t)(tmpf * TRILLION);
		if (p1->start_trigger_time <= 0.0) {
			fprintf(stderr,"%s: Invalid starttrigger time: %f. This value must be greater than 0.0\n",
				xgp->progname, tmpf);
			return(0);
		}
		p1->trigger_types |= TRIGGER_STARTTIME;
        return(5);
	} else if (strcmp(when,"op") == 0){ /* get the number of operations to wait before triggering the other target */
		p1->start_trigger_op = atoll(argv[4]);
		if (p1->start_trigger_op <= 0) {
#ifdef WIN32
			fprintf(stderr,"%s: Invalid starttrigger op: %I64d. This value must be greater than 0\n",
#else
			fprintf(stderr,"%s: Invalid starttrigger op: %lld. This value must be greater than 0\n",
#endif
				xgp->progname, p1->start_trigger_op);
			return(0);
		}
		p1->trigger_types |= TRIGGER_STARTOP;
		return(5);    
	} else if (strcmp(when,"percent") == 0){ /* get the percentage of operations to wait before triggering the other target */
		p1->start_trigger_percent = (atof(argv[4]) / 100.0);
		if ((p1->start_trigger_percent < 0.0) || (p1->start_trigger_percent > 1.0)) {
			fprintf(stderr,"%s: Invalid starttrigger percent: %f. This value must be between 0.0 and 100.0\n",
				xgp->progname, p1->start_trigger_percent * 100.0 );
			return(0);
		}
		p1->trigger_types |= TRIGGER_STARTPERCENT;
		return(5);    
	} else if (strcmp(when,"mbytes") == 0){ /* get the number of megabytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		p1->start_trigger_bytes = (uint64_t)(tmpf * 1024*1024);
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid starttrigger mbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		p1->trigger_types |= TRIGGER_STARTBYTES;
		return(5);    
	} else if (strcmp(when,"kbytes") == 0){ /* get the number of kilobytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		p1->start_trigger_bytes = (uint64_t)(tmpf * 1024);
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid starttrigger kbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		p1->trigger_types |= TRIGGER_STARTBYTES;
		return(5);    
	} else {
		fprintf(stderr,"%s: Invalid starttrigger qualifer: %s\n",
				xgp->progname, when);
            return(0);
	}
}
/*----------------------------------------------------------------------------*/
// See description of "starttrigger" option.
int
xddfunc_stoptrigger(int32_t argc, char *argv[])
{
    int t1,t2;
    double tmpf;
    ptds_t *p1;
    char *when;

	t1 = atoi(argv[1]);
	t2 = atoi(argv[2]);
    when = argv[3];

	p1 = &xgp->ptds[t1]; 
	p1->stop_trigger_target = t2; /* The target that does the triggering has to 
									* know the target number to trigger */
	if (strcmp(when,"time") == 0){ /* get the number of seconds to wait before triggering the other target */
		p1 = &xgp->ptds[t1];
		tmpf = atof(argv[4]);
		p1->stop_trigger_time = (pclk_t)(tmpf * TRILLION);
        return(5);
	} else if (strcmp(when,"op") == 0){ /* get the number of operations to wait before triggering the other target */
		p1 = &xgp->ptds[t1];
		p1->stop_trigger_op = atoll(argv[4]);
        return(5);
	} else if (strcmp(when,"percent") == 0){ /* get the percentage of operations to wait before triggering the other target */
		p1 = &xgp->ptds[t1];
		p1->stop_trigger_percent = atof(argv[4]);
        return(5);
	} else if (strcmp(when,"mbytes") == 0){ /* get the number of megabytes to wait before triggering the other target */
		p1 = &xgp->ptds[t1];
		tmpf = atof(argv[4]);
		p1->stop_trigger_bytes = (uint64_t)(tmpf * 1024*1024);
        return(5);
	} else if (strcmp(when,"kbytes") == 0){ /* get the number of kilobytes to wait before triggering the other target */
		p1 = &xgp->ptds[t1];
		tmpf = atof(argv[4]);
		p1->stop_trigger_bytes = (uint64_t)(tmpf * 1024);
        return(5);
	} else {
		fprintf(stderr,"%s: Invalid stoptrigger qualifer: %s\n",
				xgp->progname, when);
        return(0);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_syncio(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_SYNCIO;
	xgp->syncio = atoi(argv[1]);
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_syncwrite(int32_t argc, char *argv[])
{
    int j;
    int target_number;
    ptds_t *p;

    xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->target_options |= RX_SYNCWRITE;
		return(3);
	} else { /* Make all the synwrites the same */
		for (j=0; j<MAX_TARGETS; j++)
			xgp->ptds[j].target_options |= RX_SYNCWRITE;
	}
	return(1);
}
/*----------------------------------------------------------------------------*/
// Specify a single target name
int
xddfunc_target(int32_t argc, char *argv[])
{ 
    xgp->ptds[xgp->number_of_targets].target = argv[1];
    xgp->number_of_targets++; /* if single target specified, then add 1 proc */
    return(2);
}
/*----------------------------------------------------------------------------*/
// The target target directory name
// Arguments to this function look like so:
//     -targtetdir [target #] directory_name
// Where "directory_name" is the name of the directory to use for the targets
//
int
xddfunc_targetdir(int32_t argc, char *argv[])
{
    int i;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
	    p = &xgp->ptds[target_number]; 
	    p->targetdir = argv[args+1];
        return(args+2);
    } else { /* set the target directory for all targets to the specified operation */
	    xgp->ptds[0].targetdir = argv[1];
	    for (i=1; i<MAX_TARGETS; i++) {
		    xgp->ptds[i].targetdir = xgp->ptds[0].targetdir;
	    }
        return(2);
    }
}
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks
// Arguments: -targetoffset #
// 
int
xddfunc_targetoffset(int32_t argc, char *argv[])
{
	xgp->target_offset = atoll(argv[1]);
    return(2);
}
/*----------------------------------------------------------------------------*/
// Arguments to this function look like so:
//     -targtets N t1 t2 t3 ... TN
// Where N is the number of targets
//    and t1, t2, t3,... are the target names (i.e. /dev/sd0a)
// The "-targets" option is additive. Each time it is used in a command line
// new targets are added to the list of defined targets for this run.
// The global variable "number_of_targets" is the current number of targets
// that have been defined by previous "-targets"
// If the number of targets, N, is negative (i.e. -3) then there is a single
// target name specified and it is repeated N times (i.e. 3 times). 
// For example, "-targets -3 /dev/hd1" would be the equivalent of 
// "-targets 3 /dev/hd1 /dev/hd1 /dev/hd1". This is just shorthand. 
//
int
xddfunc_targets(int32_t argc, char *argv[])
{
    int i,j,k;


    k = xgp->number_of_targets;  // Keep the current number of targets we have
    xgp->number_of_targets = atoi(argv[1]); // get the number of targets to add to the list
	if (xgp->number_of_targets < 0) { // Set all the target names to the single name specified
		i = 3; // Set for the return value
        xgp->number_of_targets *= -1; // make this a positive number 
		xgp->number_of_targets += k;  // add in the previous number of targets 
		for (j=k; j<xgp->number_of_targets; j++) { 
			/* This will add targets to the end of the current list of targets */
            xgp->ptds[j].target = argv[2];
		}
	} else { // Set all target names to the appropriate name
		i = 2; // start with the third argument  
		xgp->number_of_targets += k;  // add in the previous number of targets 
		for (j=k; j<xgp->number_of_targets; j++) { 
			/* This will add targets to the end of the current list of targets */
			xgp->ptds[j].target = argv[i];
			i++;
		}
	}
    return(i);
}
/*----------------------------------------------------------------------------*/
// The target start delay function will add a delay to each target's
// starting time. The amount of time to delay is calculated simply by 
// multiplying the specified time delay multiplier by the target number.
// For example, "-targetstartdelay 2.1" will add 0 seconds to target 0's
// start time, 2.1 seconds to target 1's start time, 4.2 seconds to target 2's
// start time (i.e. 2*2.1) and so on. 
//
int
xddfunc_targetstartdelay(int32_t argc, char *argv[])
{ 
    int j;
    double tmpf;
	pclk_t start_delay;


	tmpf = atof(argv[1]);
	start_delay = (pclk_t)(tmpf * TRILLION);
	for (j=0; j<MAX_TARGETS; j++)
		xgp->ptds[j].start_delay = start_delay * j;
	return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the throttle type and associated throttle value 
// Arguments: -throttle [target #] bw|ops|var #.#
// 
int
xddfunc_throttle(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    char *what;
    double value;
    ptds_t *p;
    int retval;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		what = argv[args+1];
		value = atof(argv[args+2]);
        retval = args+3;
    } else { /* All targets */
        p = 0;
        what = argv[1];
		value = atof(argv[2]);
        retval = 3;
    }

    if (strcmp(what, "ops") == 0) {/* Throttle the ops/sec */
        if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle of %5.2f is not valid. throttle must be a number greater than 0.00\n",xgp->progname,value);
            return(0);
        }
        if (p) {
		    p->throttle_type = RX_THROTTLE_OPS;
            p->throttle = value;
        } else { // Set for all targets
			for (j=0; j<MAX_TARGETS; j++) {
				xgp->ptds[j].throttle_type = RX_THROTTLE_OPS;
				xgp->ptds[j].throttle = value;
			}
        }
        return(retval);
    } else if (strcmp(what, "bw") == 0) {/* Throttle the bandwidth */
        if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle of %5.2f is not valid. throttle must be a number greater than 0.00\n",xgp->progname,value);
            return(0);
        }
        if (p) {
		    p->throttle_type = RX_THROTTLE_BW;
            p->throttle = value;
        } else { // Set for all targets
			for (j=0; j<MAX_TARGETS; j++) {
				xgp->ptds[j].throttle_type = RX_THROTTLE_BW;
				xgp->ptds[j].throttle = value;
			}
        }
        return(retval);
    } else if (strcmp(what, "var") == 0) { /* Throttle Variance */
		if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle variance of %5.2f is not valid. throttle variance must be a number greater than 0.00 but less than the throttle value of %5.2f\n",xgp->progname,p->throttle_variance,p->throttle);
			return(0);
		}
        if (p)
            p->throttle_variance = value;
		else  { // Set for all targets
			for (j=0; j<MAX_TARGETS; j++) {
				xgp->ptds[j].throttle_variance = value;
			}
		}		
        return(retval);
    } else {
		fprintf(xgp->errout,"%s: throttle type of of %s is not valid. throttle type must be \"ops\", \"bw\", or \"var\"\n",xgp->progname,what);
		return(0);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_timelimit(int32_t argc, char *argv[])
{
    int j;
    int args;
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = &xgp->ptds[target_number];
		p->time_limit = atoi(argv[args+1]);
        return(args+2);
	} else { /* Make all the time limits the same */
		xgp->ptds[0].time_limit = atoi(argv[1]);
		for (j=1; j<MAX_TARGETS; j++)
			xgp->ptds[j].time_limit = xgp->ptds[0].time_limit;
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_timerinfo(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_TIMER_INFO;
    return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_timeserver(int32_t argc, char *argv[])
{
    int i;

	i = 1;
	/* At this point "i" points to the ts "option" argument */
	if (strcmp(argv[i], "host") == 0) { /* The host name of the time server */
        i++;
		xgp->gts_hostname = argv[i];
		return(i+1);
	} else if (strcmp(argv[i], "port") == 0) { /* The port number to use when talking to the time server host */
        i++;
		xgp->gts_port = (in_port_t)atol(argv[i]);
		return(i+1);
	} else if (strcmp(argv[i], "bounce") == 0) { /* Specify the number of times to ping the time server */
        i++;
		xgp->gts_bounce = atoi(argv[i]);
		return(i+1);
    } else {
			fprintf(stderr,"%s: Invalid timeserver option %s\n",xgp->progname, argv[i]);
            return(0);
    } /* End of the -timeserver sub options */

}
/*----------------------------------------------------------------------------*/
 // -ts on|off|detailed|summary|oneshot
 //     output filename
int
xddfunc_timestamp(int32_t argc, char *argv[]) 
{
    int i,j;
    int args;
    int target_number;
    ptds_t *p;

	i = 1;
    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
        i += args;
		p = &xgp->ptds[target_number];
		if (target_number >= MAX_TARGETS) { /* Make sure the target number is someewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for time stamp option %s\n",
				xgp->progname, target_number, argv[i]);
            return(0);
		}
	}
	/* At this point "i" points to the ts "option" argument */
	if (strcmp(argv[i], "on") == 0) { /* set the time stamp reporting option */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options |= (TS_ON | TS_ALL);
		else p->ts_options |= (TS_ON | TS_ALL);
		return(i+1);
	} else if (strcmp(argv[i], "off") == 0) { /* Turn off the time stamp reporting option */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options &= ~TS_ON; /* Turn OFF time stamping */
		else p->ts_options &= ~TS_ON;
		return(i+1);
	} else if (strcmp(argv[i], "wrap") == 0) { /* Turn on the TS Wrap option */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options |= TS_WRAP;
		else p->ts_options |= TS_WRAP;
		return(i+1);
	} else if (strcmp(argv[i], "oneshot") == 0) { /* Turn on the TS Wrap option */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options |= TS_ONESHOT;
		else p->ts_options |= TS_ONESHOT;
		return(i+1);
	} else if (strcmp(argv[i], "size") == 0) { /* Specify the number of entries in the time stamp buffer */
        i++;
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_size = atoi(argv[i]);
		else p->ts_size = atoi(argv[i]);
		return(i+1);
	} else if ((strcmp(argv[i], "triggertime") == 0) || (strcmp(argv[i], "tt") == 0)) { /* Specify the time to start time stamping */
        i++;
		if (target_number < 0) {
			for (j=0; j<MAX_TARGETS; j++) {
				xgp->ptds[j].ts_options |= (TS_ON | TS_TRIGTIME);
				xgp->ptds[j].ts_trigtime = atoll(argv[i]);
			}
		} else {
				p->ts_options |= (TS_ON | TS_TRIGTIME);
				p->ts_trigtime = atoll(argv[i]);
		}
		return(i+1);
	} else if ((strcmp(argv[i], "triggerop") == 0) || (strcmp(argv[i], "to") == 0)) { /* Specify the operation number at which to start time stamping */
		i++;
        if (target_number < 0) {
			for (j=0; j<MAX_TARGETS; j++) {
				xgp->ptds[j].ts_options |= (TS_ON | TS_TRIGOP);
				xgp->ptds[j].ts_trigop = atoi(argv[i]);
			}
		} else {
				p->ts_options |= (TS_ON | TS_TRIGOP);
				p->ts_trigop = atoi(argv[i]);
		}
		return(i+1);
	} else if (strcmp(argv[i], "normalize") == 0) { /* set the time stamp Append Output File  reporting option */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options |= ((TS_ON | TS_ALL) | TS_NORMALIZE);
		else p->ts_options |= ((TS_ON | TS_ALL) | TS_NORMALIZE);
		return(i+1);
	} else if (strcmp(argv[i], "output") == 0) { /* redirect ts output to "filename" */
        i++;
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options |= (TS_ON | TS_ALL);
		else p->ts_options |= (TS_ON | TS_ALL);
		xgp->tsoutput_filename = argv[i];
		return(i+1);
	} else if (strcmp(argv[i], "append") == 0) { /* set the time stamp Append Output File  reporting option */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options |= ((TS_ON | TS_ALL) | TS_APPEND);
		else p->ts_options |= ((TS_ON | TS_ALL) | TS_APPEND);
		return(i+1);
	} else if (strcmp(argv[i], "dump") == 0) { /* dump a binary stimestamp file to "filename" */
        i++;
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options |= ((TS_ON | TS_ALL) | TS_DUMP);
		else p->ts_options |= ((TS_ON | TS_ALL) | TS_DUMP);
		xgp->tsbinary_filename = argv[i];
		return(i+1);
	} else if (strcmp(argv[i], "summary") == 0) { /* set the time stamp SUMMARY reporting option */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options |= ((TS_ON | TS_ALL) | TS_SUMMARY);
		else p->ts_options |= ((TS_ON | TS_ALL) | TS_SUMMARY);
		return(i+1);
	} else if (strcmp(argv[i], "detailed") == 0) { /* set the time stamp DETAILED reporting option */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].ts_options |= ((TS_ON | TS_ALL) | TS_DETAILED | TS_SUMMARY);
		else p->ts_options |= ((TS_ON | TS_ALL) | TS_DETAILED | TS_SUMMARY);
		return(i+1);
	}
    return(i+1);
}
/*----------------------------------------------------------------------------*/
 // -verify location | contents
int
xddfunc_verify(int32_t argc, char *argv[]) 
{
    int i,j;
    int args;
    int target_number;
    ptds_t *p;

    i = 1;
    args = xdd_parse_target_number(argc, &argv[0], &target_number);
	if (target_number >= 0) { /* Set this option value for a specific target */
        i += args;
		p = &xgp->ptds[target_number];
		if (target_number >= MAX_TARGETS) { /* Make sure the target number is someewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for verify option %s\n",
					xgp->progname, target_number, argv[i+1]);
            return(0);
		}
	}
    if (strcmp(argv[i], "contents") == 0) { /*  Verify the contents of the buffer */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].target_options |= RX_VERIFY_CONTENTS;
		else p->target_options |= RX_VERIFY_CONTENTS;
		return(i+1);
	} else if (strcmp(argv[i], "location") == 0) { /*  Verify the buffer location */
		if (target_number < 0) 
			for (j=0; j<MAX_TARGETS; j++) xgp->ptds[j].target_options |= RX_VERIFY_LOCATION;
		else p->target_options |= (RX_VERIFY_LOCATION | RX_SEQUENCED_PATTERN);
		return(i+1);
	} else {
			fprintf(stderr,"%s: Invalid verify suboption %s\n",xgp->progname, argv[i]);
            return(0);
    } /* End of the -verify sub options */
}
/*----------------------------------------------------------------------------*/
int
xddfunc_verbose(int32_t argc, char *argv[])
{
	xgp->global_options |= RX_VERBOSE;
    return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_version(int32_t argc, char *argv[])
{
    fprintf(stdout,"%s: Version %s - %s\n",xgp->progname, XDD_VERSION, XDD_COPYRIGHT);
    return(-1);
}
/*----------------------------------------------------------------------------*/
xddfunc_invalid_option(int32_t argc, char *argv[])
{
	fprintf(xgp->errout, "%s: Invalid option: %s (%d)\n",
		xgp->progname, argv[0], argc);
    return(0);
}
/*----------------------------------------------------------------------------*/
/* xdd_parse() - Command line parser.
 */
void
xdd_parse(int32_t argc, char *argv[])
{
	int32_t i,j,q,target_number;   /* working variable */
	ptds_t *p;
	ptds_t *psave;

	xgp->progname = argv[0];
    if (argc < 1) {
        fprintf(stderr,"Error: No command line options specified\n");
        xdd_usage(0);
        exit(0);
    }

	xgp->output = stdout;
	xgp->output_filename = "stdout";
	xgp->errout = stderr;
	xgp->errout_filename = "stderr";
	xgp->csvoutput_filename = "";
	xgp->csvoutput = NULL;
	if (argc < 2) {
			xdd_usage(0);
		exit(1);
	}
	xgp->id = (char *)malloc(MAX_IDLEN);
	if (xgp->id == NULL) {
		fprintf(xgp->errout,"%s: Cannot allocate %d bytes of memory for ID string\n",xgp->progname,MAX_IDLEN);
		exit(1);
	}
	strcpy(xgp->id,DEFAULT_ID);
	xgp->id_firsttime = 1;
	/* Initialize the ptds for every possible target */
	for (i=0; i<MAX_TARGETS; i++){
		xdd_init_ptds(&xgp->ptds[i], i);
	}
	/* Initialize the global variables */
	xgp->passes = DEFAULT_PASSES;
	xgp->passdelay = DEFAULT_PASSDELAY;
	xgp->tsbinary_filename = DEFAULT_TIMESTAMP;
	xgp->syncio = 0;
	xgp->target_offset = 0;
	xgp->number_of_targets = 0;
	xgp->global_options = 0;
	xgp->tsoutput_filename = 0;
	xgp->max_errors = 0; /* set to total_ops later when we know what it is */
	xgp->gts_hostname = 0;
	xgp->gts_addr = 0;
	xgp->gts_time = 0;
	xgp->gts_port = DEFAULT_PORT;
	xgp->gts_bounce = DEFAULT_BOUNCE;
	xgp->gts_delta = 0;
	xgp->heartbeat = 0;
	for (i = 0; i < MAX_TARGETS; i++) {
		for (j = 0; j < 2; j++) {
			xgp->QThread_Barrier[i][j].semid = 0;
		}
	}
fprintf(stderr,"     \n");
	/* parse the command line arguments */
	xdd_parse_args(argc,argv);
	if (xgp->ptds[0].target == DEFAULT_TARGET) {
		fprintf(xgp->errout,"You must specify a target device or filename\n");
		xdd_usage(0);
		exit(1);
	}
	/* Queue depth processing */
	/* For any given target, if the queue depth is greater than 1 then we create a QThread PTDS for each Qthread.
	 * Each new QThread ptds is a copy of the base QThread - QThread 0 - for the associated target. The only
	 * thing that distinguishes one QThread ptds from another is the "myqnum" member which identifies the QThread for that ptds.
	 *
	 * The number of QThreads is equal to the qdepth for that target.
	 * The main ptds for the target is QThread 0. The "nextp" member of the ptds points to the QThread 1. 
	 * The "nextp" member in the QThread 1 ptds points to QThread 2, and so on. The last QThread "nextp" member is 0.
	 * 
	 *  Target0   Target1    Target2 ..... TargetN
	 *  (QThread0)   (Qthread0)   (Qthread0)  (Qthread0)
	 *   |     |      |    |
	 *   V     V      V    V
	 *   QThread1    QThread1   NULL    QThread1
	 *   |     |        |
	 *   V     V        V
	 *   QThread2    QThread2       QThread2
	 *   |     |        |
	 *   V     V        V
	 *   QThread3   NULL       QThread3
	 *   |           |
	 *   V           V
	 *  NULL          QThread4
	 *             |
	 *             V
	 *  
	 * The above example shows that Target0 has a -queuedepth of 4 and thus
	 * has 4 QThread (0-3). Similarly, Target 1 has a -queuedepth of 3 (QThreads 0-2), Target3 has a queuedepth
	 * of 1 (the default) and hence has no QThreads other than 0. Target N has a queuedepth of 5, possibly more.  
	 *  
	 */
	/* For each target check to see if we need to add ptds structures for queue-depths greater than 1 */
	xgp->number_of_iothreads = 0;
	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {

		p = &xgp->ptds[target_number];
		
		if (p->mbytes > 0) {
			p->numreqs = p->mbytes;
			p->numreqs *= (1024*1024); // This is the number of actual bytes to transfer 
			p->numreqs /= (p->reqsize * p->block_size); // This is the number of requests to perform not including the last request that might be small
		} else if (p->kbytes > 0) { /* this could be a problem */
			p->numreqs = p->kbytes;
			p->numreqs *= 1024; // This is the number of actual bytes to transfer 
			p->numreqs /= (p->reqsize * p->block_size); // This is the number of requests to perform not including the last request that might be small
		}
		if ((p->numreqs > 0) && (p->numreqs < p->queue_depth)) {
			fprintf(xgp->errout,"%s: Target %d Number of requests is too small to use with a queuedepth of %d\n",xgp->progname, target_number, p->queue_depth);
#ifdef WIN32
			fprintf(xgp->errout,"%s: queuedepth will be reset to be equal to the number of requests: %I64d\n",xgp->progname, p->numreqs);
#else
			fprintf(xgp->errout,"%s: queuedepth will be reset to be equal to the number of requests: %lld\n",xgp->progname, p->numreqs);
#endif
			p->queue_depth = p->numreqs;
			p->numreqs = 1;
			p->residual_ops = 0;
		}
		else {
			p->residual_ops = p->numreqs % p->queue_depth;
			p->numreqs /= p->queue_depth;
		}

		p->mythreadnum = xgp->number_of_iothreads;
		xgp->number_of_iothreads++;
		psave = p;
		if (p->queue_depth > 1) { /* Create some more ptds structures for this target */
			for (q = 1; q < p->queue_depth; q++ ) {
				p->seekhdr.seek_pattern = "queued_interleaved";
				p->seekhdr.seek_interleave = p->queue_depth;
				p->nextp = malloc(sizeof(struct ptds));
				if (p->nextp == NULL) {
					fprintf(xgp->errout,"%s: error getting memory for ptds for target %d queue number %d\n",
						xgp->progname, p->mynum, q);
				}
				memcpy(p->nextp, psave, sizeof(struct ptds));
				p->nextp->nextp = 0; 
				p->nextp->myqnum = q;
				p->mythreadnum = xgp->number_of_iothreads;
			    if ((p->residual_ops) && (q < p->residual_ops)) { // This means that the number of requests is not an even multiple of the queue depth. Therefore some qthreads will have more requests than others to account for the difference.
					p->nextp->numreqs++; 
				}
				xgp->number_of_iothreads++;
				p = p->nextp;
			}
		    p = &xgp->ptds[target_number]; // reset p-> to the primary target PTDS
			if (p->residual_ops) { // This means that the number of requests is not an even multiple of the queue depth. Therefore some qthreads will have more requests than others to account for the difference.
					p->numreqs++; 
			}
		}
	}
} /* end of xdd_parse() */
/*----------------------------------------------------------------------------*/
/* xdd_usage() - Display usage information |
 */
void
xdd_usage(int32_t fullhelp)
{
    int i,j;

    fprintf(stderr,"Usage: %s command-line-options\n",xgp->progname);
    i = 0;
    while(xdd_func[i].func_name) {
        fprintf(stderr,"%s",xdd_func[i].help);
        if (fullhelp) {
            j=0;
            while (xdd_func[i].ext_help[j] && (j < XDD_EXT_HELP_LINES)) {
                fprintf(stderr,"%s",xdd_func[i].ext_help[j]);
                j++;
            }
            fprintf(stderr,"\n");
        }
        i++;
    }
}
/*----------------------------------------------------------------------------*/
/* xdd_process_paramfile() - process the parameter file. 
 */
int32_t
xdd_process_paramfile(char *fnp) {
	int32_t  i;   /* working variable */
	int32_t  fd;   /* file descriptor for paramfile */
	int32_t  newsize;
	int32_t  bytesread; /* The number of bytes actually read from the param file */
	int32_t  argc;
	char *argv[8192];
	char *cp;
	char *parambuf;
	struct stat statbuf;
	/* open the parameter file */
	fd = open(fnp,O_RDONLY);
	if (fd < 0) {
		fprintf(xgp->errout, "%s: could not open parameter file '%s'\n",
			xgp->progname, fnp);
		perror("  reason");
		return(-1);
	}
	/* stat the file to get the file size in bytes */
	i = fstat(fd, &statbuf);
	if (i < 0) {
		fprintf(xgp->errout, "%s: could not stat parameter file '%s'\n",
			xgp->progname, fnp);
		perror("  reason");
		return(-1);
	}
	/* allocate a buffer for the file */
	parambuf = malloc(statbuf.st_size*2);
	if (parambuf == 0) {
		fprintf(xgp->errout,"%s: could not allocate %d bytes for parameter file\n",
			xgp->progname, (int32_t)statbuf.st_size);
		perror("  reason");
		return(-1);
	}
	memset(parambuf,0,statbuf.st_size*2);
	/* read in the entire commands file. The "bytesread" is used as the real size of the file since
	 * in *some* brain-damaged operating systems the number of bytes read is not equal to the
	 * file size reported by fstat.
	 */
	bytesread = read(fd,parambuf,statbuf.st_size*2);
	if (bytesread < 0) {
		fprintf(xgp->errout, "%s: Error: only read %d bytes of %d from parameter file\n",
			xgp->progname, bytesread, (int32_t)statbuf.st_size);
		perror("  reason");
		return(-1);
	}
	/* close the parameter file */
	close(fd);
	/* set argv[0] to the program name for consistency */
	argv[0] = xgp->progname;
	argc = 0;
	cp = parambuf;
	i = 0;
	if ((*cp == ' ') || (*cp == '\t') || (*cp == '\n')) {
		while (((*cp == ' ') || (*cp == '\t') || (*cp != '\n')) && 
			(i < bytesread)) {
			cp++;
			i++;
		}
	}
	/* at this point i points to the start of the first token in the file */
	if (i >= bytesread) return(SUCCESSFUL); /* This means that it was essentially an empty file */
	newsize = bytesread - i;
	for (i = 0; i < newsize; i++) {
		argc++;
		argv[argc] = cp;
		/* skip over the token */
		while ((*cp != ' ') && (*cp != '\t') && (*cp != '\n') && 
			(i < newsize)) {
			cp++;
			i++;
		}
		*cp = '\0';
		if (i >= newsize) break;
		cp++;
		/* skip over blanks looking for start of next token */
		while (((*cp == ' ') || (*cp == '\t') || (*cp == '\n')) && 
			(i < newsize)) {
			cp++;
			i++;
		}
		if (i >= newsize) break;
	}
	argc++;   /* add one for the 0th argument */
	xdd_parse_args(argc,argv);
	return(SUCCESSFUL);
} /* end of xdd_process_param() */
/*----------------------------------------------------------------------------*/
/* xdd_parse_target_number() - return the target number for this option.
 * The calling routine passes in the number of arguments left in the command
 * line. If the number of arguments remaining is 0 then we will not check 
 * to see if there are any other suboptions beacuse this would cause a segv.
 * In this case, we just set the target number to -1 (all targets) and return 0.
 *
 * The calling routine passes in a pointer to the first argument of the option.
 * For example, if the option is "-reqsize target 3 1024" then the "so" pointer
 * will point to the word "target". 
 * The calling routine also passes a pointer to the place to return the target
 * number. This routine will set the target number to the correct value and
 * return the number of arguments that were processed. 
 * 
 * If the "target" suboption is specified for this option then return that 
 * number as the target_number. In the example above  "target_number"
 * will be set to 3 and a value of 2 will be returned.
 *
 * If the "previous" or "prev" or "p" suboption is specified then return
 * the number of the last target specified by the "-targets 1" option.
 * For example, "-reqsize prev 1024" will set "target_number" to the
 * current number of targets that have been specified minus 1 and return 1.
 *
 * If there is no "target" or "previous" suboption then return a -1 as 
 * the target number. For example, if the option "-reqsize 1024" is specified
 * then "target_number" is set to -1 and a value of 0 will be returned.
 *
 */
int
xdd_parse_target_number(int32_t argc, char *argv[], int *target_number)
{

    if (argc <= 1) {
        *target_number = -1;
        return(0);
    }
	if (strcmp(argv[1], "target") == 0) {
        if (argc < 3) {
            fprintf(xgp->errout,"%s: ERROR: No target number specified for option %s\n",xgp->progname, argv[0]);
            return(-1);
        } else {
		*target_number = atoi(argv[2]);
        }
        return(2);
    } else if ((strcmp(argv[0], "previous") == 0) ||
               (strcmp(argv[0], "prev") == 0) ||
               (strcmp(argv[0], "p") == 0)) {
        *target_number = xgp->number_of_targets - 1;
        return(1);
    } else {
		*target_number = -1;
		return(0);
	}
	
} /* end of xdd_parse_target_number() */

#if (LINUX)
/*----------------------------------------------------------------------------*/
/* xdd_linux_cpu_count() - return the number of CPUs on  this system
 */
int32_t
xdd_linux_cpu_count(void)
{
	size_t	 	cpumask_size; 	/* size of the CPU mask in bytes */
	cpu_set_t 	cpumask; 	/* mask of the CPUs configured on this system */
	int		status; 	/* System call status */
	int32_t 	cpus; 		/* the number of CPUs configured on this system */
	int32_t		j;

#ifdef LINUXUP
	return(1);
#else
#ifdef LINUX_THREAD_PROCESSOR_AFFINITY_IS_WORKING
	cpumask_size = (size_t)sizeof(cpumask);
	status = sched_getaffinity(getpid(), cpumask_size, &cpumask);
	if (status != 0) {
		fprintf(xgp->errout,"%s: WARNING: Error getting the CPU mask - running on a single processor\n",xgp->progname);
		perror("Reason");
		cpus = 1;
	} else {
		cpus = 0;
		for (j=0; j<sizeof(cpumask)*8; j++ ) { /* Increment the cpus by 1 for every bit that is on in the cpu mask */
			if (CPU_ISSET(j,&cpumask)) cpus++;
		}
	}
	return(cpus);
#else
	return(1);
#endif  // LINUX_THREAD_PROCESSOR_AFFINITY_IS_WORKING
#endif
} /* end of xdd_linux_cpu_count() */
#endif


/*----------------------------------------------------------------------------*/
/* xdd_atohex() - convert the ascii string into real hex numbers.
 * If any of the ascii digits are not in the 0-9, a-f range then substitute
 * zeros. If there are an odd number of ascii digits then add leading zeros.
 * The way this routine works is to copy the incoming ASCII string into a temp
 * buffer and perform the conversions from that buffer into the  buffer
 * pointed to by "destp". That way the actual hex data is where it should be 
 * when we are finished. The source buffer is not touched. This routine 
 * returns the length of the hex data destination buffer in bytes.
 */
int32_t
xdd_atohex(char *destp, char *sourcep)
{
	int		i;
    int     length;
	int		hi;
	char    *cp;
	unsigned char *ucp;
	unsigned char *tmpp;


	length = strlen(sourcep);
	tmpp = (unsigned char *)malloc((size_t)length+2);
	memset(tmpp, '\0', (size_t)length+2);
	if (length % 2) 
		 strcpy(tmpp,"0"); // Odd length so start with zero
	else strcpy(tmpp,""); // terminate with a null character
	strcat(tmpp,sourcep); // put the string in the tmp area
	length = strlen(tmpp); // Get the real length of the string 
	memset(destp, '\0', length); // clear out the destination area

	hi = 1; // Indicates we are on the high-order nibble of the byte
	ucp = (unsigned char *)destp; // moving pointer to destination 
	for (i=0, cp=tmpp; i < length; i++, cp++) {
		switch(*cp) {
			case '0':
				break;
			case '1':
				if (hi) *ucp |= 0x10; else *ucp |= 0x10;
				break;
			case '2':
				if (hi) *ucp |= 0x20; else *ucp |= 0x02;
				break;
			case '3':
				if (hi) *ucp |= 0x30; else *ucp |= 0x03;
				break;
			case '4':
				if (hi) *ucp |= 0x40; else *ucp |= 0x04;
				break;
			case '5':
				if (hi) *ucp |= 0x50; else *ucp |= 0x05;
				break;
			case '6':
				if (hi) *ucp |= 0x60; else *ucp |= 0x06;
				break;
			case '7':
				if (hi) *ucp |= 0x70; else *ucp |= 0x07;
				break;
			case '8':
				if (hi) *ucp |= 0x80; else *ucp |= 0x08;
				break;
			case '9':
				if (hi) *ucp |= 0x90; else *ucp |= 0x09;
				break;
			case 'a':
				if (hi) *ucp |= 0xa0; else *ucp |= 0x0a;
				break;
			case 'b':
				if (hi) *ucp |= 0xb0; else *ucp |= 0x0b;
				break;
			case 'c':
				if (hi) *ucp |= 0xc0; else *ucp |= 0x0c;
				break;
			case 'd':
				if (hi) *ucp |= 0xd0; else *ucp |= 0x0d;
				break;
			case 'e':
				if (hi) *ucp |= 0xe0; else *ucp |= 0x0e;
				break;
			case 'f':
				if (hi) *ucp |= 0xf0; else *ucp |= 0x0f;
				break;
			case 'A':
				if (hi) *ucp |= 0xa0; else *ucp |= 0x0a;
				break;
			case 'B':
				if (hi) *ucp |= 0xb0; else *ucp |= 0x0b;
				break;
			case 'C':
				if (hi) *ucp |= 0xc0; else *ucp |= 0x0c;
				break;
			case 'D':
				if (hi) *ucp |= 0xd0; else *ucp |= 0x0d;
				break;
			case 'E':
				if (hi) *ucp |= 0xe0; else *ucp |= 0x0e;
				break;
			case 'F':
				if (hi) *ucp |= 0xf0; else *ucp |= 0x0f;
				break;
			default:
				fprintf(xgp->errout, "%s: illegal hex character of %c speficied, using 0 instead\n",xgp->progname, *cp);
				break;
		}
		hi ^= 1;
		if (hi) ucp++;
	}
	free(tmpp);
	return(length/2); // The length is the number of nibbles, length/2 is the number of bytes

} /* end of xdd_atohex() */
