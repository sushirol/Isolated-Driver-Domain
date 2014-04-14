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
 * This file contains the subroutines necessary to implement the HPSS 
 * third-party read and write operation semantics. If the HPSS global
 * variable is not set then the subroutines get compiled as null functions
 * and simply return 0.
 */
#include "xdd.h"

/* Specific to HPSS */
#if (HPSS)
#include "hpss_api.h"
#include "u_signed64.h"
#include "mvr_protocol.h"
#include "support.h"
#endif


/*----------------------------------------------------------------------------*/
/* xdd_hpss_control() - Controls the hpss_mover_managers
 *      
 *      For best performance, the request size should match either the VV block
 *      size or the Mover buffer size, whichever is less, and the maximum number
 *      of queue threads should be equal to the total number of devices the file
 *      is spread across.
 */
void *
xdd_hpss_control(void *junk) {
#if !(HPSS)
    return(0);
#else
	return(0);
#endif
} /* end of xdd_hpss_control() */
/*----------------------------------------------------------------------------*/
/* xdd_hpss_mover_manager() - hpss_mover_manager
 */
void *
xdd_hpss_mover_manager(void *junk) {
#if !(HPSS)
    return(0);
#else
	return(0);
#endif
	
} /* end of xdd_hpss_mover_manager() */
#ifdef notdef
//========================================================WRITELIST========================================================
/*==============================================================================
 *
 * Name:
 *      writelist.c - Write an HPSS file in parallel using hpss_Writelist, multiple
 *                   data-receiving threads, and the mover protocol routines,
 *                   supporting TCP, IPI-3, and shared-memory data transfers
 *       
 * Disclaimer:
 *      This software is provided "as is" and may be freely copied and modified
 *      as desired.
 *
 * Usage:
 *      writelist [-vcxl] [-n maxConnections] [-s bufferSize] 
 *               [-C cos] [-S filesize] [-h hostname] [-p tcp|shm|ipi] <path> 
 *
 *      -v             Prints verbose output
 *      -c             Prints low-level control information
 *      -t             Print transfer rate in whole bytes per second
 *      -l             IO buffer is allocated in shared memory to prevent
 *                     VM latencies
 *
 *      -n maxConnections
 *                     Maximum number of open Mover transfer connections 
 *                     (default is DEFAULT_MAX_CONNECTIONS).  
 *                     
 *      -s bufferSize  Buffer size used to send data within each transfer
 *                     thread (default is defined by DEFAULT_BUFFER_SIZE).
 *                     
 *      -C cos         Class of Service to use for creating file.  Default
 *                     is to select one based on file size.
 *                     
 *      -S filesize    Total number of bytes to write.
 *                     (default is defined by DEFAULT_FILE_SIZE).
 *                     
 *      -h hostname
 *                     Specifies one or more hostnames associated with the
 *                     desired network interface(s) for TCP-based transfers
 *                     (default is network associated with default hostname).
 *                     If multiple hostnames are specified, each transfer
 *                     threads will be assigned one of the network interfaces in
 *                     a round-robin fashion.
 *
 *      -p tcp|shm|san
 *                     By default, TCP, SAN3P and shared memory transfers are
 *                     enabled.  The -p option can be repeatedly used to specify
 *                     which transfer options to make available.  For example,
 *                     to restrict transfers to TCP only, use "-p tcp".  To make
 *                     both TCP and SAN3P available as options (but not shared
 *                     memory), use "-p tcp -p san".
 *
 *      <path>         The HPSS file to write.  Relative pathnames are resolved
 *                     from the perspective of the user's home directory within
 *                     HPSS. 
 *
 * Description:
 *
 *      This program writes all data stored in an HPSS file named <path> using
 *      parallel I/O via the hpss_WriteList API and HPSS Mover protocol
 *      functions.  The program negotiates with the corresponding HPSS Movers to
 *      determine which transfer protocol to use.  The application is coded to
 *      handle TCP, IPI-3, and/or shared memory transfers.
 *
 *      The -v option enables the user to see each transfer of data from a
 *      Mover, the order the data is sent, and what protocol is used.  The
 *      -c option shows control debug information.
 *
 *      The -t argument can be used to output a throughput number that can be
 *      more easily used by other programs, spreadsheets, etc.
 *
 *      The -s argument defines the size of each memory buffer used to send
 *      data within each transfer thread.  If not specified, a default value is
 *      used. 
 *
 *      The -n argument defines the maximum number of simultaneous connections
 *      to HPSS Movers, which also corresponds to the number of memory buffers
 *      used in receiving data in parallel.  If not specified, a default value
 *      is used.  This program will create a contiguous shared-memory segment to
 *      send data, the size of which is <bufferSize> times <maxConnections>
 *      (regardless of which transfer protocol is selected).
 *
 *      Since segments of an HPSS bitfile may be striped across multiple devices
 *      (and Movers) and/or may reside at different levels in a hierarchy,
 *      multiple buffers/threads can be used to send data in parallel to
 *      different Movers who are trying to send data independently.
 *
 *      The -h option is used to specify an alternate hostname interface(s) to
 *      use for TCP-based data transfers.
 *      
 *      For best performance, the buffer size should match either the VV block
 *      size or the Mover buffer size, whichever is less, and the maximum number
 *      of connections should be equal to the total number of devices the file
 *      is spread across.
 *
 *============================================================================*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>

#include "hpss_api.h"
#include "u_signed64.h"
#include "mvr_protocol.h"

#include "support.h"


/* Define program default values */

#define DEFAULT_BUFFER_SIZE        (4*1048576)
#define DEFAULT_MAX_CONNECTIONS    32
#define DEFAULT_SOCKET_SBMAX       (8*1048576)
#define DEFAULT_FILE_SIZE          (100*1048576)

/* Define maximum values for sanity checks */

#define MAX_BUFFER_SIZE            (128*1048576)
#define MAX_MAX_CONNECTIONS        32

/* Define local function prototypes */

void manage_mover_connections ();
void transfer_routine  (int socketDes);
void signal_thread ();
void handle_signals ();
void init_buf(char *Buf,int Size);

/* Define a structure of information to track for each Mover transfer 
 * connection/thread
 */
typedef struct {
   int       active;           /* Whether thread/connection is active */
   pthread_t threadId;         /* Id of the transfer thread */
   int       controlSocketFd;  /* Control socket descriptor */
   int       ipiFd;            /* IPI-3 transfer file descriptor */
   int       shmId;            /* Shared memory segment id */
} connection_info_t;

/* Define global variables (globals start with capital letter)
 */
int         RequestId;        /* HPSS request id */
int         TransferStatus;   /* Overall status of the data transfer 
                               * (HPSS_E_NOERROR if ok) */
int         FileDes;          /* HPSS file descriptor */
int         ControlSocket = 0;/* Central Mover connection socket */
sigset_t    SigMask;          /* Signal mask */
pthread_t   SigThreadId;      /* Signal thread id */

/* Define global variables associated with command-line options
 */
typedef struct {
   struct hostent *hostEntry;
   char            hostname[128];
   unsigned long   ipAddr;
} tcphost_t;

unsigned32  NumHosts;         /* -h argument counter */
tcphost_t  *HostList;         /* -h argument (length is NumHosts) */
int         VerboseOutput = 0;/* -v argument (0=off, 1=on) */
int         ControlOutput = 0;/* -c argument (0=off, 1=on) */
int         WholeBytesOutput = 0;/* -x argument (0=off, 1=on) */
unsigned32  MaxConnections;   /* -n argument */
unsigned32  BufferSize;       /* -s argument */
u_signed64  FileSize;         /* Size of the HPSS file */
int         ShmBuffer = 0;      /* -l argument */

/* The following global variables are protected by the mutex GlobalVarMutex 
 */
pthread_mutex_t GlobalVarMutex = HPSS_PTHREAD_MUTEX_INITIALIZER;

connection_info_t  *Connections = NULL; /* Array of connection thread info */

int         CurrentHost = 0;        /* Index into HostList for next TCP address */
u_signed64  TotalBytesWritten;        /* Actual bytes sent to Movers */

/*==============================================================================
 * Function:
 *    hpss_error - Print out info on an HPSS error condition
 *
 * Arguments:
 *    function     Name of the HPSS function that returned "status"
 *    status       HPSS error code
 *
 * Return Values:
 *    <none>
 *============================================================================*/

void hpss_error (char *function, signed32 status)
{
   fprintf (stderr, "%s (%ld): %s\n", function, status, status_string (status));
}

extern void exit_routine(void);

/*==============================================================================
 * Function:
 *    terminate - Gracefully terminate the process, closing resources
 *                appropriately 
 *
 * Arguments:
 *    exitStatus  Value used as the exit status
 *
 * Return Values:
 *    This function terminates the process and therefore does not
 *    return.
 *============================================================================*/

void terminate (int exitStatus)
{
   int index;
   /* Close down the HPSS file if it is open
    */
   if (FileDes > 0) {
      if (ControlOutput) printf("Closing HPSS file descriptor %d\n", FileDes);
      (void)hpss_Close (FileDes);
   }
   if ( Connections != NULL )
   {
      /* Step through the connections and for any that are active, delete the
       * shared memory segment if it exists
       */
      for (index=0; index < MaxConnections; index++) {

         if (Connections[index].active && Connections[index].shmId != -1) {
            if (ControlOutput) 
               printf("Deleting shared memory for thread %d\n", index+1);
            shmctl (Connections[index].shmId, IPC_RMID, (struct shmid_ds *)NULL);
         }
      }
   }
   exit (exitStatus);
}

/*==============================================================================
 * main
 *============================================================================*/
  
main (int argc, char *argv[])
{
   int                   rc, i, badUsage; /* Counters and flags */
   size_t                tmp;        /* Temporary variables */
   int                   cos=0;      /* Specified COS (if any) */
   char                  *programName, *s;
   hpss_IOD_t            iod;  /* IOD passed to hpss_WriteList */
   hpss_IOR_t            ior;  /* IOR returned from hpss_WriteList */
   iod_srcsinkdesc_t     src, sink; /* IOD source/sink descriptors */
   hpss_cos_hints_t      hints;
   hpss_cos_priorities_t pris;
   hpss_cos_md_t         cosinfo;
   struct sockaddr_in    controlSocketAddr; /* control socket addresses */
   int                   writeListFlags = 0; /* Flags on hpss_WriteList call */
   pthread_t             manageConnectionsThread; /* Spawned thread id */

   void                  *pthreadStatus;
   signed32              status;        /* HPSS return code/status */
   timestamp_t           startTime, endTime, totalTime; /* various timestamps */

   totalTime.tv_sec = totalTime.tv_usec = 0;
   RequestId = getpid();

   memset (&iod,  0, sizeof(iod));
   memset (&src,  0, sizeof(src));
   memset (&sink, 0, sizeof(sink));
   memset(&hints,0,sizeof(hints));
   memset(&pris,0,sizeof(pris));
   memset (&ior,  0, sizeof(ior));
   programName     = argv[0];
   badUsage        = 0;
   MaxConnections  = DEFAULT_MAX_CONNECTIONS;
   BufferSize      = DEFAULT_BUFFER_SIZE;
   FileSize        = cast64m(DEFAULT_FILE_SIZE);
   src.Flags      = 0;

   /* Process the arguments
    */

   while (--argc > 0 && (*++argv)[0] == '-') {

      for (s = argv[0]+1; *s != '\0'; s++) {
      
         switch (*s) {

            case 'v':                 /* for verbose output */
               VerboseOutput = 1;
               break;

            case 'c':                 /* for low-level control output */
               ControlOutput = 1;
               break;

            case 't':                 /* for printing whole byte throughput rate */
               WholeBytesOutput = 1;
               break;

            case 'l':                 /* for shared memory buffer */
               ShmBuffer = 1;
	       break;

            case 'n':                 /* to specify max connections/no. of buffers */
               if (argc > 1 && (*(argv+1))[0] != '-') {
                  MaxConnections = atoi((++argv)[0]);
                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            case 's':                 /* to specify buffer size */
               if (argc > 1 && (*(argv+1))[0] != '-') {
                  BufferSize = atobytes((++argv)[0]);
                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            case 'S':                 /* to specify file size */
               if (argc > 1 && (*(argv+1))[0] != '-') {
                  FileSize =  atobytes64((++argv)[0],&rc);
                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            case 'C':                 /* to specify class of service */
               if (argc > 1 && (*(argv+1))[0] != '-') {
                  cos = atoi((++argv)[0]);
                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            case 'h':                 /* TCP hostname */
               if (argc > 1 && (*(argv+1))[0] != '-') {
                  if (!HostList) {
                     HostList = (tcphost_t *)malloc (sizeof(*HostList));
                  }
                  else {
                     HostList = (tcphost_t *)realloc (HostList, sizeof(*HostList) *
                                                      (NumHosts + 1));
                  }
          
                  strncpy (HostList[NumHosts].hostname, (++argv)[0], 
                           sizeof(HostList[NumHosts].hostname));

                  /* Make sure it is a legitimate hostname */

                  HostList[NumHosts].hostEntry = 
                     gethostbyname (HostList[NumHosts].hostname);

                  if (!(HostList[NumHosts].hostEntry)) {
                     fprintf (stderr, "Invalid hostname \"%s\"\n",
                              HostList[NumHosts].hostname);
                     perror ("gethostbyname");
                     badUsage = 1;
                  }
                  else {
                     HostList[NumHosts].ipAddr = 
                        *((unsigned32*)(HostList[NumHosts].hostEntry->h_addr_list[0]));
                     ++NumHosts;
                  }

                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            case 'p':                 /* transfer protocol */
               if (argc > 1) {
                  ++argv;

                  if (!strcmp(argv[0], "tcp")) {
                     src.Flags |= HPSS_IOD_XFEROPT_IP;
                  }
                  else if (!strcmp(argv[0], "shm")) {
                     src.Flags |= HPSS_IOD_XFEROPT_SHMEM;
                  }
                  else if (!strcmp(argv[0], "san")) {
                     src.Flags |= HPSS_IOD_XFEROPT_SAN3P;
                  }
                  else {
                     printf ("Invalid transfer protocol - use tcp, shm, or ipi\n");
                     badUsage = 1;
                  }
                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            default:
               badUsage = 1;

         } /* end switch */
      } /* end for */
   } /* end while */

   if (argc != 1) badUsage = 1;

   if (badUsage) {
      printf("Usage:\n\n");
      printf("%s [-vct] [-n maxConnections] [-s bufferSize]\n",
             programName);
      printf("         [-C cos] [-S filesize] [-h hostname]*\n");
      printf("         [-p tcp|shm|ipi]* [-x sbmax] <path>\n\n");
      printf("-v\n");
      printf("\tPrints verbose output.\n\n");
      printf("-c\n");
      printf("\tPrints low-level control information.\n\n");
      printf("-t\n");
      printf("\tPrints throughput rate in whole bytes.\n\n");
      printf("-n maxConnections\n");
      printf("\tMaximum number of concurrent transfer threads that are available for\n");
      printf("\tcommunicating simultaneously with HPSS Movers.  This corresponds to the\n");
      printf("\tnumber of buffers allocated to send HPSS data.  Default is %d.\n\n",
             DEFAULT_MAX_CONNECTIONS);
      printf("-s bufferSize\n");
      printf("\tBuffer size used to send data within each transfer thread.  Values\n");
      printf("\tsuch as \"2mb\" can be specified.  Default is ");
      print_bytes (DEFAULT_BUFFER_SIZE);
      printf(".\n\n");
      printf("-C cos\n");
      printf("\tClass of Service to use for creating file.\n");
      printf("\tDefault is to select one based on file size.\n\n");
      printf("-S filesize\n");
      printf("\tTotal number of files to write. Values such as \"2mb\" can be specified.\n");
      printf("\tDefault is ");
      print_bytes (DEFAULT_FILE_SIZE);
      printf(".\n\n");
      printf("-h hostname\n");
      printf("\tSpecifies one or more hostnames associated with the desired network\n");
      printf("\tinterface(s) for TCP-based transfers (default is network associated\n");
      printf("\twith default hostname).  If multiple hostnames are specified, each\n");
      printf("\ttransfer thread will be assigned one of the network interfaces in a\n");
      printf("\tround-robin fashion.\n\n");
      printf("-p tcp|shm|san\n");
      printf("\tBy default, TCP, SAN3P and shared memory transfers are\n");
      printf("\tenabled.  The -p option can be repeatedly used to specify\n");
      printf("\twhich transfer options to make available.\n");
      printf("<path>\n");
      printf("\tThe HPSS file to write.  Relative pathnames are resolved from the\n");
      printf("\tperspective of the user's home directory within HPSS.\n");

      terminate (1);
   }


   /* Perform some sanity checks on values
    */
   if (MaxConnections > MAX_MAX_CONNECTIONS) { 
      printf ("Maximum limit on number of buffers is %d\n", MAX_MAX_CONNECTIONS);
      terminate (1);
   }

   if (BufferSize > MAX_BUFFER_SIZE) {
      printf ("Maximum limit on buffer size is ");
      print_bytes (MAX_BUFFER_SIZE);
      printf ("\n");
      terminate (1);
   }
   /* If no transfer protocol(s) were specified, use all of them (use IPI-3
    * only if it was compiled in)
    */
   if (!src.Flags) {
      src.Flags  = HPSS_IOD_XFEROPT_IP | 
                   HPSS_IOD_XFEROPT_SHMEM |
                   HPSS_IOD_XFEROPT_SAN3P;
   }
   src.Flags |= HPSS_IOD_HOLD_RESOURCES | HPSS_IOD_CONTROL_ADDR;

   /* If no hostname was specified, use the local default hostname for TCP 
    * transfers
    */
   if (!HostList) {
      HostList = (tcphost_t *) malloc (sizeof(*HostList));
          
      if (gethostname (HostList[0].hostname, 
                       sizeof(HostList[0].hostname)) < 0) {
         perror ("gethostname");
         terminate (1);
      }

      HostList[0].hostEntry = gethostbyname (HostList[0].hostname);

      if (!(HostList[NumHosts].hostEntry)) {
         perror ("gethostbyname");
         terminate (1);
      }
      HostList[0].ipAddr = 
         *((unsigned32*)(HostList[0].hostEntry->h_addr_list[0]));
      ++NumHosts;
   }

   /* Set up signal handling
    */
   handle_signals();

   /* Allocate the array of transfer/connection information
    */
   Connections = (connection_info_t *) malloc (sizeof(Connections[0]) * 
                                               MaxConnections);
   memset (Connections, 0, sizeof(Connections[0]) * MaxConnections);

   /* Pass the COS as a hint, if it was specified.  If not specified, a
    * COS will be selected automatically based on file size.  Even if
    * was give a COS as a hint, still provide file size info so a 
    * proper segment size will be selected.
    */
   if (cos) {
      hints.COSId = cos;
      pris.COSIdPriority = REQUIRED_PRIORITY;
   }
   hints.MinFileSize = FileSize;
   hints.MaxFileSize = FileSize;
   pris.MinFileSizePriority = REQUIRED_PRIORITY;
   pris.MaxFileSizePriority = REQUIRED_PRIORITY;

   /* Open the HPSS file (argv[0] points to the HPSS pathname)
    */
   FileDes = hpss_Open (argv[0], O_WRONLY | O_TRUNC | O_CREAT, 
                        0666, &hints, &pris, NULL);
  
   if (FileDes < 0) {
      hpss_error ("hpss_Open", FileDes);
      terminate (FileDes);
   } 
   if (VerboseOutput) {    
      printf("File size is ");
      print_bytes64(FileSize);
      putchar('\n');
   }

    
   /* Create the local control socket which all Movers will initially connect to
    */
   ControlSocket = socket (AF_INET, SOCK_STREAM, 0);

   if (ControlSocket == -1) {
      perror ("socket");
      terminate (1);
   }
    
   if (ControlOutput) {
      printf ("Control socket is socket %d\n", ControlSocket);
   }

   (void)memset (&controlSocketAddr, 0, sizeof(controlSocketAddr));
   controlSocketAddr.sin_family      = AF_INET;
   controlSocketAddr.sin_addr.s_addr = INADDR_ANY;
   controlSocketAddr.sin_port        = 0;

   if (bind (ControlSocket, (const struct sockaddr*)&controlSocketAddr, 
             sizeof(controlSocketAddr)) == -1) {
      perror ("bind");
      terminate (1);
   }

   tmp = sizeof (controlSocketAddr);

   if (getsockname (ControlSocket, 
                    (struct sockaddr *)&controlSocketAddr, 
                    (sockapi_len_t *)&tmp) == -1) {
      perror ("getsockname");
      terminate (1);
   }
    
   if (listen (ControlSocket, SOMAXCONN) == -1) {
      perror ("listen");
      terminate (1);
   }

   /* Start the thread to receive control connections from individual Movers
    */

   /* Spawn a thread to handle this transfer request
    */
   rc=pthread_create (&manageConnectionsThread,
                      &hpss_pthread_attr_default,
                      (void *(*)(void *)) manage_mover_connections,
                      (void *)NULL);
   if ( rc != 0 )
   {
      printf("pthread_attr_create: %d\n",rc);
      terminate(1);
   }
   pthread_yield();

   /* Set the source/sink length to the number of bytes we want
    */
   sink.Offset = src.Offset = cast64m(0);
   sink.Length = src.Length = FileSize;

   /* Define source and sink descriptors and the IOD
    */
   sink.SrcSinkAddr.Type = CLIENTFILE_ADDRESS;
   sink.SrcSinkAddr.Addr_u.ClientFileAddr.FileDes = FileDes;
   sink.SrcSinkAddr.Addr_u.ClientFileAddr.FileOffset = cast64m(0);
    
   src.SrcSinkAddr.Type = NET_ADDRESS;
   src.SrcSinkAddr.Addr_u.NetAddr.SockTransferID  = cast64m (RequestId);
   src.SrcSinkAddr.Addr_u.NetAddr.SockAddr.addr   = HostList[0].ipAddr;
   src.SrcSinkAddr.Addr_u.NetAddr.SockAddr.port   = controlSocketAddr.sin_port;
   src.SrcSinkAddr.Addr_u.NetAddr.SockAddr.family = controlSocketAddr.sin_family;
   src.SrcSinkAddr.Addr_u.NetAddr.SockOffset      = cast64m (0);
    
   iod.Function       = HPSS_IOD_WRITE;
   iod.RequestID      = RequestId;
   iod.SrcDescLength  = 1;
   iod.SinkDescLength = 1;
   iod.SrcDescList    = &src;
   iod.SinkDescList   = &sink;


   if (ControlOutput) {
      printf("Client request id is %d\n", RequestId);
   }

   TotalBytesWritten = cast64m(0);
                                   
   startTime = get_current_timestamp();
   TransferStatus = HPSS_E_NOERROR;

   if (ControlOutput) {
      printf ("Issuing hpss_WriteList call for ");
      print_bytes64 (FileSize);
      printf ("\n");
   }

   memset (&ior,  0, sizeof(ior));

   status = hpss_WriteList (&iod, writeListFlags, &ior);
   if (status) {
      hpss_error ("hpss_WriteList", status);

      if (ior.Status != HPSS_E_NOERROR) {
         hpss_error ("IOR status", ior.Status);
         printf ("Returned flags is 0x%x, bytes moved is ", ior.Flags);
         print_bytes64 (TotalBytesWritten);
         putchar ('\n');
         terminate (1);
      }

      if (TransferStatus == HPSS_E_NOERROR) TransferStatus = status;
   }

   endTime = get_current_timestamp ();

   totalTime = diff_timestamps (startTime, endTime);

   /* Close the HPSS file
    */
   status = hpss_Close (FileDes);

   if (status < 0) {
      hpss_error ("hpss_Close", status);
   }

   /* Let's make sure that all data has actually been sent before we kill
    * any active transfer threads
    */
   pthread_mutex_lock (&GlobalVarMutex);

   if (neq64m (FileSize, TotalBytesWritten)) {

      struct timespec delay = { 1, 0 }; /* 1 second */

      printf("filesize is ");
      print_bytes64(FileSize);
      printf(", TotalBytesWritten is ");
      print_bytes64(TotalBytesWritten);
      printf("\n");

      pthread_mutex_unlock (&GlobalVarMutex);

      /* Wait for all transfer threads to complete before moving on
       */

      for (tmp=0, i=0; i < MaxConnections; i++) {
         pthread_mutex_lock (&GlobalVarMutex);

         while (Connections[i].active) {
            pthread_mutex_unlock (&GlobalVarMutex);

            if ((VerboseOutput || ControlOutput) && !tmp) {
               printf ("Waiting on thread %d to complete...\n", i+1);
               tmp = 1;                /* only show the message once */
            }
            (void) pthread_delay (&delay);

            pthread_mutex_lock (&GlobalVarMutex);
         } /* end of while */
         pthread_mutex_unlock (&GlobalVarMutex);
      }
   }
   /* Print stats
    */
   if (!eq64m (TotalBytesWritten, cast64m(0))) {
      u_signed64 usecs64;
      unsigned32 throughput;

      usecs64    = add64m (mul64m (cast64m (totalTime.tv_sec), 1000000),
                           cast64m (totalTime.tv_usec));
                  
      throughput = cast32m (div2x64m (mul64m (TotalBytesWritten, 1000000), 
                                      usecs64));

      if (WholeBytesOutput) {
         printf ("%ld\n", throughput);
      }
      else {
         print_bytes64 (TotalBytesWritten);
         printf(" successfully written in %d.%06d sec -> ",
                totalTime.tv_sec, totalTime.tv_usec);
      
         usecs64 = mul64m (cast64m (totalTime.tv_sec), 1000000);
         inc64m (usecs64, cast64m (totalTime.tv_usec));
      
         print_bytes_per_second (cast32m (div2x64m (mul64m (TotalBytesWritten, 1000000), 
                                                    usecs64)));
         putchar('\n');
      }
      fflush (stdout);
   }
   pthread_mutex_unlock (&GlobalVarMutex);

   /* Now cancel the manage_mover_connections thread
    */
   (void)pthread_cancel (manageConnectionsThread);
   (void)pthread_cancel (SigThreadId);
   terminate (0);
}

/*==============================================================================
 * Function:
 *    manage_mover_connections - Accept socket connections from HPSS Movers and
 *                               spawn a new thread to handle each Mover
 *                               connection and data transfer
 * Return Values:
 *    <none>
 *============================================================================*/

void manage_mover_connections ()
{
   int                 moverSocketFd; /* New Mover socket file descriptor */
   int                 index;         /* Counters */
   int                 rc, tmp;       /* Temporary variable */
   pthread_t           self;
   struct sockaddr_in  socketAddr;

   self = pthread_self();
   (void)pthread_detach(self);

   /* Loop until this thread is cancelled
    */
   for (;;) {

      tmp = sizeof(socketAddr);

      while ((moverSocketFd = 
              accept (ControlSocket, (struct sockaddr *)&socketAddr, 
                      (sockapi_len_t *)&tmp)) < 0) {

         if ((errno != EINTR) && (errno != EAGAIN)) {
            perror ("accept");
            TransferStatus = errno;
            break;
         }

      } /* end while */

      
      if (moverSocketFd < 0) break;

      if (ControlOutput) {
         printf ("Mover control connection accepted on control socket %d\n", 
                 moverSocketFd);
      }
      if (VerboseOutput) {
         printf ("Peer Address = ");
         print_peer (stdout,moverSocketFd);
         printf ("\n");
      }

      /* Find a connection/transfer thread that is free to accept this
       * connection.  If one is not free, sleep for a bit
       * and try again.
       */
      do {

         socket_setoptions (moverSocketFd, NULL);
         if (VerboseOutput)
            check_sockopts("control socket options", moverSocketFd);

         pthread_mutex_lock (&GlobalVarMutex);

         for (index = 0; index < MaxConnections; index++) {

            if (!Connections[index].active) {
               Connections[index].active = 1;
               Connections[index].controlSocketFd = moverSocketFd;
               break;
            }
         }
         pthread_mutex_unlock (&GlobalVarMutex);
      
         /* 
          * Sleep (without blocking the process) if no
          * free buffer/thread was found
          */
         if (index == MaxConnections) {

            struct timespec delay = { 0, 500000 };
            (void) pthread_delay (&delay);
         }
      
      } while (index == MaxConnections);
      
      /* Spawn a thread to handle this transfer request
       */
      rc=pthread_create (&Connections[index].threadId,
                         &hpss_pthread_attr_default,
                         (void *(*)(void *)) transfer_routine,
                         (void *) index);
      if ( rc != 0 )
      {
         printf("pthread_attr_create: %d\n",rc);
         terminate(1);
      }
      pthread_yield();

   } /* end for */

   return;
}

/*==============================================================================
 * Function:
 *   handle_signals - Routine to cause process to catch common signals and
 *                    gracefully terminate
 *============================================================================*/

void handle_signals()
{
   int rc;

   sigemptyset (&SigMask);
   sigaddset (&SigMask, SIGHUP);
   sigaddset (&SigMask, SIGINT);
   sigaddset (&SigMask, SIGQUIT);
   sigaddset (&SigMask, SIGTERM);

#if !defined(LINUX)
   (void) sigprocmask (SIG_SETMASK, &SigMask, (sigset_t *)NULL);
#endif

   /* Spawn a thread to handle this transfer request
    */
   rc=pthread_create (&SigThreadId,
                      &hpss_pthread_attr_default,
                      (void *(*)(void *)) signal_thread,
                      (void *)NULL);
   if ( rc != 0 )
   {
      printf("pthread_attr_create: %d\n",rc);
      terminate(1);
   }
   pthread_yield();
}
  
/*==============================================================================
 * Function:
 *   signal_thread - Thread to catch signals and gracefully terminate the 
 *                   process by removing any allocated shared memory
 *============================================================================*/

void signal_thread()
{
   int index = 0;
   int status = 0;

#if defined(LINUX)
   (void) pthread_sigmask (SIG_SETMASK, &SigMask, (sigset_t *)NULL);
#endif

#if !defined(__sgi) && !defined(_IBMR2) && !defined(LINUX)
   status = sigwait(&SigMask);
#else
   do {
      status = sigwait (&SigMask,&status);
   } while(status == EINTR);
#endif

   /* Step through the connections and for any that are active, delete the
    * shared memory segment if it exists
    */

   if (ControlOutput) printf("****** signal received ******\n");

   for (index=0; index < MaxConnections; index++) {

      printf("Connections[%d].active=%d\n",index,Connections[index].active);
      printf("Connections[%d].shmId=%d\n",index,Connections[index].shmId);
      if (Connections[index].active && Connections[index].shmId != -1) {
         if (ControlOutput) printf("Deleting shared memory for thread %d\n", index+1);
         shmctl (Connections[index].shmId, IPC_RMID, (struct shmid_ds *)NULL);
         free_shm(NULL,Connections[index].shmId);
      }
   }
   exit (status);
}

/*==============================================================================
 * Function:
 *   transfer_routine - Send data transfer using mover protocol
 *
 * Arguments:
 *   index  - Index into Connections array for this thread
 *
 * Return Values:
 *    <none>
 *============================================================================*/

void transfer_routine (int index)
{
   int                   status, tmp; /* Return, temporary values */
   int                   transferListenSocket; /* Socket listen descriptors */
   int                   transferSocketFd; /* Transfer accept socket */
   struct sockaddr_in    transferSocketAddr; /* Transfer socket address */
   int                   bytesSent;
   initiator_msg_t       initMessage, initReply;
   initiator_ipaddr_t    ipAddr;        /* TCP socket address info */
   initiator_shmaddr_t   shmAddr;        /* Shared memory address info */
   initiator_san3paddr_t sanAddr;  /* SAN3P address info */
   completion_msg_t      completionMessage;
   char                  *buffer;        /* Transfer data buffer */
   char                  *freeBuffer;  /* Transfer data buffer */

   if (ControlOutput)
      printf("Thread %d - Started, using control socket %d\n", index+1,
             Connections[index].controlSocketFd);

   Connections[index].shmId = -1;
   transferListenSocket = transferSocketFd = -1;

   buffer = freeBuffer = NULL;

   /* Loop until we reach a condition to discontinue talking with Mover
    */
   while (TransferStatus == HPSS_E_NOERROR) {

      /* Get the next transfer initiation message from the Mover.
       * HPSS_ECONN will be returned when the Mover is done.
       */
      status = mvrprot_recv_initmsg (Connections[index].controlSocketFd,
                                     &initMessage);

      if (ControlOutput) printf("Thread %d - mvrprot_recv_initsg returned %ld\n", 
                                index+1, status);

      if (status == HPSS_ECONN) {
         if (ControlOutput) printf("Connection closed by mover...\n");
         break;                        /* break out of the while loop */
      }
      else if (status != HPSS_E_NOERROR) {

         hpss_error ("mvrprot_recv_initmsg returned", status);
         TransferStatus = status;
         continue;
      }

      if (ControlOutput) {
         printf ("Thread %d - Mover ready to accept ", index+1);
         print_bytes64 (initMessage.Length);
         printf (" at offset ");
         print_bytes64 (initMessage.Offset);
         printf (" via %s\n",
                 initMessage.Type == NET_ADDRESS ? "TCP" :
                 initMessage.Type == SHM_ADDRESS ? "SHM" : "SAN3P");
      }

      /* Tell the Mover we will send the address next
       */
      initReply.Flags = MVRPROT_COMP_REPLY | MVRPROT_ADDR_FOLLOWS;

      /* Let's agree to use the transfer protocol selected by the
       * Mover and let's accept the offset.  However, the number
       * of bytes the Mover can transfer at one time is limited
       * by our buffer size, so we tell the Mover how much of the
       * data he has offerred that we are willing to accept.
       */
      initReply.Type   = initMessage.Type;
      initReply.Offset = initMessage.Offset;
    
      if (gt64m (initMessage.Length, cast64m(BufferSize)))
         initReply.Length = cast64m(BufferSize);
      else
         initReply.Length = initMessage.Length;

      /* Send our response back to the Mover
       */
      status = mvrprot_send_initmsg (Connections[index].controlSocketFd,
                                     &initReply);
    
      if (status != HPSS_E_NOERROR) {
         hpss_error ("mvrprot_send_initmsg", status);
         TransferStatus = status;
         continue;
      }

      /* Based on the type of transfer protocol, allocate memory, send address
       * information, and send the data to the HPSS Mover
       */
      switch (initMessage.Type) {

         case SHM_ADDRESS:

            /* If we have not already created the shared memory segment for this
             * thread, do it now 
             */
            if (!buffer) {

               Connections[index].shmId = shmget (IPC_PRIVATE, BufferSize,
                                                  S_IRWXU | S_IRWXG | S_IRWXO);
               if (Connections[index].shmId == -1) {
                  perror ("shmget");
                  TransferStatus = errno;
                  continue;
               }
    
               buffer = shmat (Connections[index].shmId, NULL, 0);

               if (!buffer) {
                  perror ("shmat");
                  TransferStatus = errno;
                  continue;
               }

               init_buf(buffer,BufferSize);

               memset (&shmAddr, 0, sizeof(shmAddr));
               shmAddr.Flags             = 0;
               shmAddr.ShmAddr.Flags     = 0;
               shmAddr.ShmAddr.ShmID     = Connections[index].shmId;
               shmAddr.ShmAddr.ShmOffset = 0;
            }

            /* Tell the Mover what our shared memory address is
             */
            status = mvrprot_send_shmaddr (Connections[index].controlSocketFd,
                                           &shmAddr);

            if (status != HPSS_E_NOERROR) {
               hpss_error ("mvrprot_send_shmaddr", status);
               TransferStatus = status;
               continue;
            }

            /* At this point, the Mover is taking the data out of shared memory
             * and we wait for the completion message
             */
            break;
          
         case NET_ADDRESS:

            /* The first time through, allocate the memory buffer and data transfer
             * socket
             */
            if (!buffer) {
	       if(ShmBuffer) {
                  buffer = malloc_shm(BufferSize, &Connections[index].shmId);
	       }
	       else {
                  buffer = valloc (BufferSize); 
                  if (!buffer) {
                     perror ("malloc");
                     TransferStatus = errno;
                     continue;
                  }
               }

               transferListenSocket = socket (AF_INET, SOCK_STREAM, 0);

               if (transferListenSocket == -1) {
                  perror ("socket");
                  TransferStatus = errno;
                  continue;
               }
    
               if (ControlOutput) {
                  printf ("Thread %d - Opened transfer listen socket %d\n", index+1,
                          transferListenSocket);
               }

               (void)memset (&transferSocketAddr, 0, sizeof(transferSocketAddr));
               transferSocketAddr.sin_family      = AF_INET;
               transferSocketAddr.sin_port        = 0;

               /* Select the hostname (IP address) in a round-robin fashion
                */
               pthread_mutex_lock (&GlobalVarMutex);

               transferSocketAddr.sin_addr.s_addr = HostList[CurrentHost++].ipAddr;
               if (CurrentHost == NumHosts) CurrentHost = 0;

               pthread_mutex_unlock (&GlobalVarMutex);

               if (bind (transferListenSocket,
                         (const struct sockaddr*)&transferSocketAddr, 
                         sizeof(transferSocketAddr)) == -1) {
                  perror ("bind");
                  return;
               }

               tmp = sizeof (transferSocketAddr);

               (void)memset (&transferSocketAddr, 0, sizeof(transferSocketAddr));

               if (getsockname (transferListenSocket, 
                                (struct sockaddr *)&transferSocketAddr, 
                                (sockapi_len_t *)&tmp) == -1) {
                  perror ("getsockname");
                  TransferStatus = errno;
                  continue;
               }
    
               if (listen (transferListenSocket, SOMAXCONN) == -1) {
                  perror ("listen");
                  TransferStatus = errno;
                  continue;
               }

               if (VerboseOutput) {
                  printf ("Thread %d - Using TCP network address %d.%d.%d.%d:%d\n", 
                          index+1,
                          (transferSocketAddr.sin_addr.s_addr & 0xff000000) >> 24,
                          (transferSocketAddr.sin_addr.s_addr & 0x00ff0000) >> 16,
                          (transferSocketAddr.sin_addr.s_addr & 0x0000ff00) >> 8,
                          (transferSocketAddr.sin_addr.s_addr & 0x000000ff),
                          transferSocketAddr.sin_port);
               }
        
               memset (&ipAddr,  0, sizeof(ipAddr));
               ipAddr.IpAddr.SockTransferID  = cast64m (RequestId);
               ipAddr.IpAddr.SockAddr.family = transferSocketAddr.sin_family;
               ipAddr.IpAddr.SockAddr.addr   = transferSocketAddr.sin_addr.s_addr;
               ipAddr.IpAddr.SockAddr.port   = transferSocketAddr.sin_port;
               ipAddr.IpAddr.SockOffset      = cast64m (0);
            }

            /* Tell the Mover what socket to receive the data from
             */
            status = mvrprot_send_ipaddr (Connections[index].controlSocketFd, &ipAddr);
      
            if (status != HPSS_E_NOERROR) {
               hpss_error ("mvrprot_send_ipaddr", status);
               TransferStatus = status;
               continue;
            }
      
            /* Wait for the new Mover socket connection, if you don't already
             * have one
             */
            if (transferSocketFd == -1) {

               tmp = sizeof(transferSocketAddr);
        
               while ((transferSocketFd = 
                       accept (transferListenSocket, 
                               (struct sockaddr *)&transferSocketAddr, 
                               (sockapi_len_t *)&tmp)) < 0) {
          
                  if ((errno != EINTR) && (errno != EAGAIN)) {
                     TransferStatus = errno;
                     break;
                  }
               } /* end while */

               if (ControlOutput)
                  printf("Thread %d - accept received, new transfer"
                         " socket is %d\n", 
                         index+1, transferSocketFd);

               if (VerboseOutput)
               {
                  printf ("Peer Address = ");
                  print_peer (stdout,transferSocketFd);
                  printf ("\n");
               }

               socket_setoptions (transferSocketFd, NULL);
               if (VerboseOutput)
                  check_sockopts("data socket options", transferSocketFd);

               if (transferSocketFd < 0) continue;

            }

            /* Send the data to the Mover via our socket
             */
            status = mover_socket_send_requested_data(transferSocketFd, 
                                                      cast64m (RequestId),
                                                      initMessage.Offset,
                                                      buffer,
                                                      low32m(initReply.Length),
                                                      &bytesSent, 1);
            if (status <= 0) {
               hpss_error ("mover_socket_send_requested_data", status);
               TransferStatus = status;
            }
            break;

         case SAN3P_ADDRESS:

            /* The first time through, allocate the memory buffer
	     * and data transfer socket
             */
            if (!buffer) {
	       if(ShmBuffer) {
                  buffer = malloc_shm(BufferSize, &Connections[index].shmId);
	       }
	       else {
                  status = hpss_PAMalloc(BufferSize,&freeBuffer,&buffer);
	          if(status) {
		     freeBuffer = buffer = NULL;
                     hpss_error("hpss_PAMalloc", status);
                     TransferStatus = status;
                     continue;
                  }
               }
            }

            /* Build/send the SAN3P initiator address message */
            sanAddr.Flags = 0;
            sanAddr.San3pAddr.SAN3PTransferID = (unsigned32)RequestId;
            memset(&sanAddr.San3pAddr.SanSecurityToken,0x0,
                  sizeof(san_sec_token_t));

            status = mvrprot_send_san3paddr(Connections[index].controlSocketFd,
					    &sanAddr);
            if (status != HPSS_E_NOERROR) {
               hpss_error ("mvrprot_send_san3paddr", status);
               TransferStatus = status;
               continue;
            }

	    /* Read our data */
            status = san3p_Write((san3paddress_t *)&sanAddr,
                                 Connections[index].controlSocketFd,
		                 buffer,
                                 low32m(initReply.Length),
				 (unsigned int *)&bytesSent);

            /* Send a completion message back to the mover */
            completionMessage.Flags = 0;
            completionMessage.Status = status;
            completionMessage.BytesMoved = cast64m(bytesSent);
            memcpy((char *)completionMessage.SecurityTicket,
                   (char *)initMessage.SecurityTicket,
                   MVRPROT_TICKET_LENGTH);
            tmp = mvrprot_send_compmsg(Connections[index].controlSocketFd,
                                       &completionMessage);
			   
            if (status != HPSS_E_NOERROR) {
               hpss_error ("san3p_Read", status);
               TransferStatus = status;
               continue;
            }

            if (tmp != HPSS_E_NOERROR) {
               hpss_error ("mvrprot_send_compmsg", status);
               TransferStatus = status;
               continue;
            }

            break;          

         default:
            break;

      } /* end switch */

      /*
       * Get a transfer completion message from the Mover
       */
      status = mvrprot_recv_compmsg (Connections[index].controlSocketFd,
                                     &completionMessage);

      if (status != HPSS_E_NOERROR) {
         hpss_error ("mvrprot_recv_compmsg", status);
         TransferStatus = status;
         continue;
      } 
      
      if (VerboseOutput) {
         printf ("Thread %d - ", index+1);
      
         print_bytes64 (completionMessage.BytesMoved);
         printf(" sent at offset ");
         print_bytes64 (initMessage.Offset);
         printf(" via ");
      
         switch (initMessage.Type) {
            case SHM_ADDRESS:
               printf ("SHM\n");
               break;
            case NET_ADDRESS:
               printf ("TCP\n");
               break;
            case SAN3P_ADDRESS:
               printf ("SAN3P\n");
               break;
            default:
               break;
         }
      }

      pthread_mutex_lock (&GlobalVarMutex);
      inc64m (TotalBytesWritten, completionMessage.BytesMoved);
      if (ge64m(TotalBytesWritten,FileSize))
      {
         TransferStatus = 0;
         pthread_mutex_unlock(&GlobalVarMutex);
         break;
      }
      pthread_mutex_unlock (&GlobalVarMutex);
      
   } /* end while loop */

   if (ControlOutput) {
      printf("Closing down thread %d\n", index+1);
   }

   /* Clean up, based on the transfer protocol
    */
   switch (initMessage.Type) {

      case SHM_ADDRESS:

         /* Remove the shared memory segment if it got allocated
          */
         if (Connections[index].shmId != -1) {
            shmdt (buffer);
            shmctl (Connections[index].shmId, IPC_RMID, (struct shmid_ds *)NULL);
            Connections[index].shmId = -1;
         }
         break;

      case SAN3P_ADDRESS:
         /* Free the buffer if it was allocated
          */
	 if(ShmBuffer) { 
            free_shm(buffer,Connections[index].shmId);
	 }
	 else if (buffer && freeBuffer) {
	    (void) free(freeBuffer);
         }
         break;

      case NET_ADDRESS:
      default:

         /* Close down the TCP transfer socket if it got opened
          */
         if (transferSocketFd != -1) {
            (void) close(transferSocketFd);
         }

         /* Free the buffer if it was allocated
          */
	 if(ShmBuffer) { 
            free_shm(buffer,Connections[index].shmId);
	 }
	 else if (buffer) {
	    (void) free(buffer);
         }

         break;

   } /* end switch */

   /* Close the control socket
    */
   (void) close (transferListenSocket);

   /* Close the control socket and mark this connection as not
    * active
    */
   pthread_mutex_lock (&GlobalVarMutex);

   (void) close (Connections[index].controlSocketFd);

   Connections[index].active = 0;

   pthread_mutex_unlock (&GlobalVarMutex);

   return;
}

void init_buf(char *Buf,int Size)
{
   int cnt;
   char *bufptr;

   bufptr = Buf;
   for (cnt = 0; cnt < Size; cnt += 4)
   {
      *(int *)bufptr = cnt / 4;
      bufptr += 4;
   }
}
//================================================READLIST==================================================
/*==============================================================================
 *
 * Name:
 *      readlist.c - Read an HPSS file in parallel using hpss_Readlist, multiple
 *                   data-receiving threads, and the mover protocol routines,
 *                   supporting TCP, SAN3P, and shared-memory data transfers
 *       
 * Disclaimer:
 *      This software is provided "as is" and may be freely copied and modified
 *      as desired.
 *
 * Usage:
 *      readlist [-vctl] [-n maxConnections] [-s bufferSize] [-h hostname]
 *               [-p tcp|shm|ipi]* [-o] <path> 
 *
 *      -v             Prints verbose output
 *      -c             Prints low-level control information
 *      -t             Print transfer rate in whole bytes per second
 *      -l             IO buffer is allocated in shared memory to prevent
 *                     VM latencies
 *
 *      -n maxConnections
 *                     Maximum number of open Mover transfer connections 
 *                     (default is DEFAULT_MAX_CONNECTIONS).  
 *                     
 *      -s bufferSize  Buffer size used to receive data within each transfer
 *                     thread (default is defined by DEFAULT_BUFFER_SIZE).
 *                     
 *      -h hostname
 *                     Specifies one or more hostnames associated with the
 *                     desired network interface(s) for TCP-based transfers
 *                     (default is network associated with default hostname).
 *                     If multiple hostnames are specified, each transfer
 *                     threads will be assigned one of the network interfaces in
 *                     a round-robin fashion.
 *
 *      -p tcp|shm|san
 *                     By default, TCP, SAN3P and shared memory transfers are
 *                     enabled.  The -p option can be repeatedly used to specify
 *                     which transfer options to make available.  For example,
 *                     to restrict transfers to TCP only, use "-p tcp".  To make
 *                     both TCP and SAN3P available as options (but not shared
 *                     memory), use "-p tcp -p san".
 *
 *      -o             Instructs HPSS to send data to this application in
 *                     sequential order.
 *
 *      <path>         The HPSS file to read.  Relative pathnames are resolved
 *                     from the perspective of the user's home directory within
 *                     HPSS. 
 *
 * Description:
 *
 *      This program reads all data stored in an HPSS file named <path> using
 *      parallel I/O via the hpss_ReadList API and HPSS Mover protocol
 *      functions.  The program negotiates with the corresponding HPSS Movers to
 *      determine which transfer protocol to use.  The application is coded to
 *      handle TCP, IPI-3, and/or shared memory transfers.
 *
 *      The -v option enables the user to see each transfer of data from a
 *      Mover, the order the data is received, and what protocol is used.  The
 *      -c option shows control debug information.
 *
 *      The -t argument can be used to output a throughput number that can be
 *      more easily used by other programs, spreadsheets, etc.
 *
 *      The -s argument defines the size of each memory buffer used to receive
 *      data within each transfer thread.  If not specified, a default value is
 *      used. 
 *
 *      The -n argument defines the maximum number of simultaneous connections
 *      to HPSS Movers, which also corresponds to the number of memory buffers
 *      used in receiving data in parallel.  If not specified, a default value
 *      is used.  This program will create a contiguous shared-memory segment to
 *      receive data, the size of which is <bufferSize> times <maxConnections>
 *      (regardless of which transfer protocol is selected).
 *
 *      Since segments of an HPSS bitfile may be striped across multiple devices
 *      (and Movers) and/or may reside at different levels in a hierarchy,
 *      multiple buffers/threads can be used to receive data in parallel from
 *      different Movers who are trying to send data independently.
 *
 *      The -h option is used to specify an alternate hostname interface(s) to
 *      use for TCP-based data transfers.
 *
 *      The -o option causes the HPSS_READ_SEQUENTIAL flag to be used in the
 *      hpss_ReadList.  This means that at any point in time, the next byte in
 *      transfer order is being processed - not waiting for a byte later in
 *      transfer order).
 *      
 *      For best performance, the buffer size should match either the VV block
 *      size or the Mover buffer size, whichever is less, and the maximum number
 *      of connections should be equal to the total number of devices the file
 *      is spread across.
 *
 *============================================================================*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>

#include "hpss_api.h"
#include "u_signed64.h"
#include "mvr_protocol.h"

#include "support.h"


/* Define program default values */

#define DEFAULT_BUFFER_SIZE        (4*1048576)
#define DEFAULT_MAX_CONNECTIONS    32
#define DEFAULT_SOCKET_SBMAX       (8*1048576)

/* Define maximum values for sanity checks */

#define MAX_BUFFER_SIZE            (32*1048576)
#define MAX_MAX_CONNECTIONS        32

/* Define local function prototypes */

void manage_mover_connections ();
void transfer_routine  (int socketDes);
void signal_thread ();
void handle_signals ();

/* Define a structure of information to track for each Mover transfer 
 * connection/thread
 */
typedef struct {
   int       active;             /* Whether thread/connection is active */
   pthread_t threadId;           /* Id of the transfer thread */
   int       controlSocketFd;    /* Control socket descriptor */
   int       shmId;              /* Shared memory segment id */
} connection_info_t;

/* Define global variables (globals start with capital letter)
 */
int         RequestId;          /* HPSS request id */
int         TransferStatus;     /* Overall status of the data transfer 
                                 * (HPSS_E_NOERROR if ok) */
int         FileDes;            /* HPSS file descriptor */
int         ControlSocket = 0;  /* Central Mover connection socket */
sigset_t    SigMask;            /* Signal mask */

/* Define global variables associated with command-line options
 */
typedef struct {
   struct hostent *hostEntry;
   char            hostname[128];
   unsigned long   ipAddr;
} tcphost_t;

unsigned32  NumHosts;           /* -h argument counter */
tcphost_t  *HostList;           /* -h argument (length is NumHosts) */
int         VerboseOutput = 0;  /* -v argument (0=off, 1=on) */
int         ControlOutput = 0;  /* -c argument (0=off, 1=on) */
int         WholeBytesOutput = 0;/* -x argument (0=off, 1=on) */
unsigned32  MaxConnections;     /* -n argument */
unsigned32  BufferSize;         /* -s argument */
int         ShmBuffer = 0;      /* -l argument */

/* The following global variables are protected by the mutex GlobalVarMutex 
 */
pthread_mutex_t GlobalVarMutex; 

connection_info_t  *Connections = NULL; /* Array of connection thread info */

int         CurrentHost = 0;    /* Index into HostList for next TCP address */
u_signed64  TotalBytesRead;     /* Actual bytes received from Movers */
u_signed64  GapBytes;           /* Number of "gap" bytes within the file */

/*==============================================================================
 * Function:
 *    hpss_error - Print out info on an HPSS error condition
 *
 * Arguments:
 *    function     Name of the HPSS function that returned "status"
 *    status       HPSS error code
 *
 * Return Values:
 *    <none>
 *============================================================================*/

void hpss_error (char *function, signed32 status)
{
   fprintf (stderr, "%s (%ld): %s\n", function, status, status_string (status));
}

/*==============================================================================
 * Function:
 *    terminate - Gracefully terminate the process, closing resources
 *                appropriately 
 *
 * Arguments:
 *    exitStatus  Value used as the exit status
 *
 * Return Values:
 *    This function terminates the process and therefore does not
 *    return.
 *============================================================================*/

void terminate (int exitStatus)
{
   int index;

   /* Close down the HPSS file if it is open
    */
   if (FileDes > 0) {
      if (ControlOutput) printf("Closing HPSS file descriptor %d\n", FileDes);
      (void)hpss_Close (FileDes);
   }

   if ( Connections != NULL )
   {
      /* Step through the connections and for any that are active, delete the
       * shared memory segment if it exists
       */
      for (index=0; index < MaxConnections; index++) {
         if (Connections[index].active && Connections[index].shmId != -1) {
            if (ControlOutput) printf("Deleting shared memory for"
                                      " thread %d\n", index+1);
            shmctl (Connections[index].shmId, IPC_RMID,
                    (struct shmid_ds *)NULL);
         }
      }
   }

   exit (exitStatus);
}

/*==============================================================================
 * main
 *============================================================================*/
  
main (int argc, char *argv[])
{
   int                   rc, i, badUsage; /* Counters and flags */
   size_t                tmp;             /* Temporary variables */
   char                  *programName, *s;
   hpss_IOD_t            iod;    /* IOD passed to hpss_ReadList */
   hpss_IOR_t            ior;    /* IOR returned from hpss_ReadList */
   iod_srcsinkdesc_t     src, sink; /* IOD source/sink descriptors */
   struct sockaddr_in    controlSocketAddr; /* control socket addresses */
   int                   readListFlags = 0; /* Flags on hpss_ReadList call */
   u_signed64            fileSize; /* Size of the HPSS file */
   u_signed64            bytesMoved; /* Bytes transferred, from IOR */
   pthread_t             manageConnectionsThread; /* Spawned thread id */
   void                  *pthreadStatus;
   pthread_mutexattr_t   mutexattr;
   pthread_attr_t        threadattr;
   signed32              status; /* HPSS return code/status */
   timestamp_t           startTime, endTime, totalTime; /* various timestamps */

   totalTime.tv_sec = totalTime.tv_usec = 0;

   memset (&iod,  0, sizeof(iod));
   memset (&src,  0, sizeof(src));
   memset (&sink, 0, sizeof(sink));
    
   programName     = argv[0];
   badUsage        = 0;
   MaxConnections  = DEFAULT_MAX_CONNECTIONS;
   BufferSize      = DEFAULT_BUFFER_SIZE;
   sink.Flags      = 0;


   /* Process the arguments
    */

   while (--argc > 0 && (*++argv)[0] == '-') {

      for (s = argv[0]+1; *s != '\0'; s++) {
      
         switch (*s) {

            case 'v':                 /* for verbose output */
               VerboseOutput = 1;
               break;

            case 'c':                 /* for low-level control output */
               ControlOutput = 1;
               break;

            case 't':                 /* for printing whole byte throughput rate */
               WholeBytesOutput = 1;
               break;

            case 'l':                 /* for shared memory buffer */
               ShmBuffer = 1;
	       break;

            case 'o':                 /* to get data in parallel sequential order */
               readListFlags = HPSS_READ_SEQUENTIAL;
               break;

            case 'n':                 /* to specify max connections/no. of buffers */
               if (argc > 1 && (*(argv+1))[0] != '-') {
                  MaxConnections = atoi((++argv)[0]);
                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            case 's':                 /* to specify buffer size */
               if (argc > 1 && (*(argv+1))[0] != '-') {
                  BufferSize = atobytes((++argv)[0]);
                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            case 'h':                 /* TCP hostname */
               if (argc > 1 && (*(argv+1))[0] != '-') {
                  if (!HostList) {
                     HostList = (tcphost_t *)malloc (sizeof(*HostList));
                  }
                  else {
                     HostList = (tcphost_t *)realloc (HostList, sizeof(*HostList) *
                                                      (NumHosts + 1));
                  }
          
                  strncpy (HostList[NumHosts].hostname, (++argv)[0], 
                           sizeof(HostList[NumHosts].hostname));

                  /* Make sure it is a legitimate hostname */

                  HostList[NumHosts].hostEntry = 
                     gethostbyname (HostList[NumHosts].hostname);

                  if (!(HostList[NumHosts].hostEntry)) {
                     fprintf (stderr, "Invalid hostname \"%s\"\n",
                              HostList[NumHosts].hostname);
                     perror ("gethostbyname");
                     badUsage = 1;
                  }
                  else {
                     HostList[NumHosts].ipAddr = 
                        *((unsigned32*)(HostList[NumHosts].hostEntry->h_addr_list[0]));
                     ++NumHosts;
                  }

                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            case 'p':                 /* transfer protocol */
               if (argc > 1) {
                  ++argv;

                  if (!strcmp(argv[0], "tcp")) {
                     sink.Flags |= HPSS_IOD_XFEROPT_IP;
                  }
                  else if (!strcmp(argv[0], "shm")) {
                     sink.Flags |= HPSS_IOD_XFEROPT_SHMEM;
                  }
                  else if (!strcmp(argv[0], "san")) {
                     sink.Flags |= HPSS_IOD_XFEROPT_SAN3P;
                  }
                  else {
                     printf ("Invalid transfer protocol - use tcp, shm, or ipi\n");
                     badUsage = 1;
                  }
                  argc -= 1;
               }
               else
                  badUsage = 1;
               break;

            default:
               badUsage = 1;

         } /* end switch */
      } /* end for */
   } /* end while */

   if (argc != 1) badUsage = 1;

   if (badUsage) {
      printf("Usage:\n\n");
      printf("%s [-vco] [-n maxConnections] [-s bufferSize] [-h hostname]*\n",
             programName);
      printf("         [-p tcp|shm|ipi]* <path>\n\n");
      printf("-v\n");
      printf("\tPrints verbose output.\n\n");
      printf("-c\n");
      printf("\tPrints low-level control information.\n\n");
      printf("-t\n");
      printf("\tPrints throughput rate in whole bytes.\n\n");
      printf("-o\n");
      printf("\tInstructs HPSS to send data to this application in sequential parallel\n");
      printf("\torder.\n\n");
      printf("-n maxConnections\n");
      printf("\tMaximum number of concurrent transfer threads that are available for\n");
      printf("\tcommunicating simultaneously with HPSS Movers.  This corresponds to the\n");
      printf("\tnumber of buffers allocated to receive HPSS data.  Default is %d.\n\n",
             DEFAULT_MAX_CONNECTIONS);
      printf("-s bufferSize\n");
      printf("\tBuffer size used to receive data within each transfer thread.  Values\n");
      printf("\tsuch as \"2mb\" can be specified.  Default is ");
      print_bytes (DEFAULT_BUFFER_SIZE);
      printf(".\n\n");
      printf("-h hostname\n");
      printf("\tSpecifies one or more hostnames associated with the desired network\n");
      printf("\tinterface(s) for TCP-based transfers (default is network associated\n");
      printf("\twith default hostname).  If multiple hostnames are specified, each\n");
      printf("\ttransfer thread will be assigned one of the network interfaces in a\n");
      printf("\tround-robin fashion.\n\n");
      printf("-p tcp|shm|san\n");
      printf("\tBy default, TCP, SAN3P and shared memory transfers are\n");
      printf("\tenabled.  The -p option can be repeatedly used to specify\n");
      printf("\twhich transfer options to make available.\n");
      printf("<path>\n");
      printf("\tThe HPSS file to read.  Relative pathnames are resolved from the\n");
      printf("\tperspective of the user's home directory within HPSS.\n");

      terminate (1);
   }

   /* Perform some sanity checks on values
    */
   if (MaxConnections > MAX_MAX_CONNECTIONS) { 
      printf ("Maximum limit on number of buffers is %d\n", MAX_MAX_CONNECTIONS);
      terminate (1);
   }

   if (BufferSize > MAX_BUFFER_SIZE) {
      printf ("Maximum limit on buffer size is ");
      print_bytes (MAX_BUFFER_SIZE);
      printf ("\n");
      terminate (1);
   }
  
   /* If no transfer protocol(s) were specified, use all of them 
    */
   if (!sink.Flags) {
      sink.Flags  = HPSS_IOD_XFEROPT_IP | 
                    HPSS_IOD_XFEROPT_SHMEM |
                    HPSS_IOD_XFEROPT_SAN3P;
   }
   sink.Flags |= (HPSS_IOD_CONTROL_ADDR | HPSS_IOD_HOLD_RESOURCES);

   /* If no hostname was specified, use the local default hostname for TCP 
    * transfers
    */
   if (!HostList) {
      HostList = (tcphost_t *) malloc (sizeof(*HostList));
          
      if (gethostname (HostList[0].hostname, 
                       sizeof(HostList[0].hostname)) < 0) {
         perror ("gethostname");
         terminate (1);
      }

      HostList[0].hostEntry = gethostbyname (HostList[0].hostname);

      if (!(HostList[NumHosts].hostEntry)) {
         perror ("gethostbyname");
         terminate (1);
      }
      HostList[0].ipAddr = 
         *((unsigned32*)(HostList[0].hostEntry->h_addr_list[0]));
      ++NumHosts;
   }

   /* Set up signal handling
    */
   handle_signals();

   /* Allocate the array of transfer/connection information
    */
   Connections = (connection_info_t *) malloc (sizeof(Connections[0]) * 
                                               MaxConnections);
  
   memset (Connections, 0, sizeof(Connections[0]) * MaxConnections);

   rc = pthread_mutexattr_init(&mutexattr);

   if (rc)
   {
      printf("Unable to initialize mutex attrs: %ld\n", rc);
      terminate(1);
   }

   rc = pthread_mutex_init (&GlobalVarMutex, &mutexattr);
   if (rc)
   {
      printf("Unable to initialize mutex: %ld\n", rc);
      terminate(1);
   }

   /* Open the HPSS file (argv[0] points to the HPSS pathname)
    */
   FileDes = hpss_Open (argv[0], O_RDONLY, 0777, NULL, NULL, NULL);
  
   if (FileDes < 0) {
      hpss_error ("hpss_Open", FileDes);
      terminate (FileDes);
   }

   /* Get the file size by lseek'ing to the end of the file and seeing what
    * position is returned.  Then lseek back to the beginning of the file.
    */
   status = hpss_SetFileOffset (FileDes, cast64m(0), SEEK_END,
                                HPSS_SET_OFFSET_FORWARD,
                                &fileSize);
   if (status) {
      hpss_error ("hpss_SetFileOffset(end)", status);
      terminate (status);
   }

   if (VerboseOutput) {    
      printf("File size is ");
      print_bytes64(fileSize);
      putchar('\n');
   }

   status = hpss_Lseek (FileDes, 0, SEEK_SET);

   if (status) {
      hpss_error ("hpss_Lseek(0)", status);
      terminate (status);
   }

   /* Create the local control socket which all Movers will initially connect to
    */
   ControlSocket = socket (AF_INET, SOCK_STREAM, 0);

   if (ControlSocket == -1) {
      perror ("socket");
      terminate (1);
   }
    
   if (ControlOutput) {
      printf ("Control socket is socket %d\n", ControlSocket);
   }

   (void)memset (&controlSocketAddr, 0, sizeof(controlSocketAddr));
   controlSocketAddr.sin_family      = AF_INET;
   controlSocketAddr.sin_addr.s_addr = INADDR_ANY;
   controlSocketAddr.sin_port        = 0;

   if (bind (ControlSocket, (const struct sockaddr*)&controlSocketAddr, 
             sizeof(controlSocketAddr)) == -1) {
      perror ("bind");
      terminate (1);
   }

   tmp = sizeof (controlSocketAddr);

   if (getsockname (ControlSocket, 
                    (struct sockaddr *)&controlSocketAddr, 
                    (sockapi_len_t *)&tmp) == -1) {
      perror ("getsockname");
      terminate (1);
   }
    
   if (listen (ControlSocket, SOMAXCONN) == -1) {
      perror ("listen");
      terminate (1);
   }
   
   rc = pthread_attr_init(&threadattr);

   if ( rc != 0 )
   {
      printf("pthread_attr_init: %ld\n",rc);
      terminate(1);
   }

   rc = pthread_attr_setdetachstate(&threadattr,PTHREAD_CREATE_DETACHED);

   if ( rc != 0 )
   {
      printf("pthread_attr_setdetachstate: %d\n",rc);
      terminate(1);
   }

   /* Start the thread to receive control connections from individual Movers
    */
   pthread_create (&manageConnectionsThread,
                   &threadattr,
                   (void *(*)(void *)) manage_mover_connections,
                   (void *)NULL);
   pthread_yield();

   /* Define source and sink descriptors and the IOD
    */
   src.SrcSinkAddr.Type = CLIENTFILE_ADDRESS;
   src.SrcSinkAddr.Addr_u.ClientFileAddr.FileDes = FileDes;
    
   RequestId = getpid();

   sink.SrcSinkAddr.Type = NET_ADDRESS;
   sink.SrcSinkAddr.Addr_u.NetAddr.SockTransferID  = cast64m (RequestId);
   sink.SrcSinkAddr.Addr_u.NetAddr.SockAddr.addr   = HostList[0].ipAddr;
   sink.SrcSinkAddr.Addr_u.NetAddr.SockAddr.port   = controlSocketAddr.sin_port;
   sink.SrcSinkAddr.Addr_u.NetAddr.SockAddr.family = controlSocketAddr.sin_family;
   sink.SrcSinkAddr.Addr_u.NetAddr.SockOffset      = cast64m (0);
    
   iod.Function       = HPSS_IOD_READ;
   iod.RequestID      = RequestId;
   iod.SrcDescLength  = 1;
   iod.SinkDescLength = 1;
   iod.SrcDescList    = &src;
   iod.SinkDescList   = &sink;

   if (ControlOutput) {
      printf("Client request id is %d\n", RequestId);
   }

   GapBytes = TotalBytesRead = bytesMoved = cast64m(0);
                                   
   startTime = get_current_timestamp();
   TransferStatus = HPSS_E_NOERROR;

   /* Loop as long as the total bytes moved plus all reported gaps are less
    * than the total size of the file AND no transfer error has been encountered
    */
   while (lt64m(add64m(bytesMoved, GapBytes), fileSize) && 
          TransferStatus == HPSS_E_NOERROR) {

      /* Set the source/sink length to the number of bytes we want
       */
      src.Offset = sink.Offset = add64m(bytesMoved, GapBytes);
      src.Length = sink.Length = sub64m(fileSize, src.Offset);

      src.SrcSinkAddr.Addr_u.ClientFileAddr.FileOffset = src.Offset;

      if (ControlOutput) {
         printf ("Issuing hpss_ReadList call for ");
         print_bytes64 (sub64m(fileSize, add64m(bytesMoved, GapBytes)));
         printf ("\n");
      }
        
      memset (&ior,  0, sizeof(ior));
    
      status = hpss_ReadList (&iod, readListFlags, &ior);

      if (status) {
         hpss_error ("hpss_ReadList", status);

         if (ior.Status != HPSS_E_NOERROR) {
            hpss_error ("IOR status", ior.Status);
            printf ("Returned flags is 0x%x, bytes moved is ", ior.Flags);
            print_bytes64 (bytesMoved);
            putchar ('\n');
            terminate (1);
         }

         if (TransferStatus == HPSS_E_NOERROR) TransferStatus = status;
      }
      else {

         inc64m (bytesMoved, ior.SinkReplyList->BytesMoved);

         /* See if data transfer stopped at a gap (hole)
          */
         if (ior.Flags & HPSS_IOR_GAPINFO_VALID) {
        
            if (VerboseOutput) {
               printf ("GAP encountered at offset ");
               print_bytes64 (ior.ReqSpecReply->ReqReply_s.ReqReply_u.GapInfo.Offset);
               printf (", length ");
               print_bytes64 (ior.ReqSpecReply->ReqReply_s.ReqReply_u.GapInfo.Length);
               putchar ('\n');
            }
        
            inc64m (GapBytes, ior.ReqSpecReply->ReqReply_s.ReqReply_u.GapInfo.Length);
        
         }
         free_ior_mem(&ior);
      }
    
   } /* end while */

   endTime = get_current_timestamp ();

   totalTime = diff_timestamps (startTime, endTime);

   /* Close the HPSS file
    */
   status = hpss_Close (FileDes);

   if (status < 0) {
      hpss_error ("hpss_Close", status);
   }

   /* Let's make sure that all data has actually been received before we kill
    * any active transfer threads
    */
   pthread_mutex_lock (&GlobalVarMutex);

   if (neq64m (bytesMoved, TotalBytesRead)) {

      struct timespec delay = { 1, 0 }; /* 1 second */

      printf("bytesMoved is ");
      print_bytes64(bytesMoved);
      printf(", TotalBytesRead is ");
      print_bytes64(TotalBytesRead);
      printf("\n");

      pthread_mutex_unlock (&GlobalVarMutex);

      /* Wait for all transfer threads to complete before moving on
       */
      for (tmp=0, i=0; i < MaxConnections; i++) {

         pthread_mutex_lock (&GlobalVarMutex);

         while (Connections[i].active) {

            pthread_mutex_unlock (&GlobalVarMutex);

            if ((VerboseOutput || ControlOutput) && !tmp) {
               printf ("Waiting on thread %d to complete...\n", i+1);
               tmp = 1;              /* only show the message once */
            }
            (void) pthread_delay(&delay);
            pthread_mutex_lock (&GlobalVarMutex);
         }
         pthread_mutex_unlock (&GlobalVarMutex);
      }
   }
  
   /* Print stats
    */
   if (!eq64m (TotalBytesRead, cast64m(0))) {

      u_signed64 usecs64;
      unsigned32 throughput;

      usecs64    = add64m (mul64m (cast64m (totalTime.tv_sec), 1000000),
                           cast64m (totalTime.tv_usec));
                  
      throughput = cast32m (div2x64m (mul64m (TotalBytesRead, 1000000), 
                                      usecs64));

      if (WholeBytesOutput) {
         printf ("%ld\n", throughput);
      }
      else {
         print_bytes64 (TotalBytesRead);
         printf(" successfully read in %d.%06d sec -> ",
                totalTime.tv_sec, totalTime.tv_usec);
      
         print_bytes_per_second (throughput);

         putchar('\n');
    
         if (neqz64m (GapBytes)) {
            printf ("...");
            print_bytes64 (GapBytes);
            printf (" within gaps\n");
         }
      }
      fflush (stdout);
   }
   pthread_mutex_unlock (&GlobalVarMutex);

   /* Now cancel the manage_mover_connections thread
    */
   (void)pthread_cancel (manageConnectionsThread);

   terminate (0);
}

/*==============================================================================
 * Function:
 *    manage_mover_connections - Accept socket connections from HPSS Movers and
 *                               spawn a new thread to handle each Mover
 *                               connection and data transfer
 * Return Values:
 *    <none>
 *============================================================================*/

void manage_mover_connections ()
{
   int                 moverSocketFd;  /* New Mover socket file descriptor */
   int                 index;          /* Counters */
   int                 rc, tmp;        /* Temporary variable */
   pthread_attr_t      threadattr;
   pthread_t           self;
   void                *status;
   struct sockaddr_in  socketAddr;

   self = pthread_self();

   (void)pthread_detach(self);

   /* Loop until this thread is cancelled
    */
   for (;;) {

      tmp = sizeof(socketAddr);

      while ((moverSocketFd = 
              accept (ControlSocket, (struct sockaddr *)&socketAddr, 
                      (sockapi_len_t *)&tmp)) < 0) {

         if ((errno != EINTR) && (errno != EAGAIN)) {
            perror ("accept");
            TransferStatus = errno;
            break;
         }

      } /* end while */
      
      if (moverSocketFd < 0) break;

      if (ControlOutput)
         printf ("Mover control connection accepted on control socket %d\n", 
                 moverSocketFd);

      if (VerboseOutput) {
         printf ("Peer Address = "); 
         print_peer(stdout,moverSocketFd);
         printf ("\n");
      }

      /* Find a connection/transfer thread that is free to accept this
       * connection.  If one is not free, sleep for a bit
       * and try again.
       */
      do {

         socket_setoptions (moverSocketFd, NULL);
         if (VerboseOutput)
            check_sockopts("control socket options", moverSocketFd);

         pthread_mutex_lock (&GlobalVarMutex);

         for (index = 0; index < MaxConnections; index++) {

            if (!Connections[index].active) {
               Connections[index].active = 1;
               Connections[index].controlSocketFd = moverSocketFd;
               break;
            }
         }
         pthread_mutex_unlock (&GlobalVarMutex);
      
         /* Sleep (without blocking the process) if no free buffer/thread was found
          */
         if (index == MaxConnections) {
            struct timespec delay = { 0, 500000 };
            (void) pthread_delay(&delay);
         }
      
      } while (index == MaxConnections);
      

      rc = pthread_attr_init(&threadattr);

      if ( rc != 0 )
      {
         printf("pthread_attr_init: %ld\n",rc);
         terminate(1);
      }

      rc = pthread_attr_setdetachstate(&threadattr,PTHREAD_CREATE_DETACHED);

      if ( rc != 0 )
      {
         printf("pthread_attr_setdetachstate: %d\n",rc);
         terminate(1);
      }

      /* Spawn a thread to handle this transfer request
       */
      pthread_create (&Connections[index].threadId,
                      &threadattr,
                      (void *(*)(void *)) transfer_routine,
                      (void *) index);
      pthread_yield();

   } /* end for */
  
   return;
}

/*==============================================================================
 * Function:
 *   handle_signals - Routine to cause process to catch common signals and
 *                    gracefully terminate
 *============================================================================*/

void handle_signals()
{
   int rc;
   pthread_t threadId;           /* Signal thread id */
   pthread_attr_t threadattr;

   sigemptyset (&SigMask);
   sigaddset (&SigMask, SIGHUP);
   sigaddset (&SigMask, SIGINT);
   sigaddset (&SigMask, SIGQUIT);
   sigaddset (&SigMask, SIGTERM);

#if !defined(LINUX)
   (void) sigprocmask (SIG_SETMASK, &SigMask, (sigset_t *)NULL);
#endif

   rc = pthread_attr_init(&threadattr);

   if ( rc != 0 )
   {
      printf("pthread_attr_init: %ld\n",rc);
      terminate(1);
   }

   rc = pthread_attr_setdetachstate(&threadattr,PTHREAD_CREATE_DETACHED);
   if ( rc != 0 )
   {
      printf("pthread_attr_setdetachstate: %d\n",rc);
      terminate(1);
   }


   /* Spawn a thread to catch signals
    */
   pthread_create (&threadId,
                   &threadattr,
                   (void *(*)(void *)) signal_thread,
                   (void *) NULL);
   pthread_yield();
}
  
/*==============================================================================
 * Function:
 *   signal_thread - Thread to catch signals and gracefully terminate the 
 *                   process by removing any allocated shared memory
 *============================================================================*/

void signal_thread()
{
   int index;
   int status;

#if defined(LINUX)
   (void) pthread_sigmask (SIG_SETMASK, &SigMask, (sigset_t *)NULL);
#endif

   status = sigwait(&SigMask,&status);

   /* Step through the connections and for any that are active, delete the
    * shared memory segment if it exists
    */

   if (ControlOutput) printf("****** signal received ******\n");

   for (index=0; index < MaxConnections; index++) {

      if (Connections[index].active && Connections[index].shmId != -1) {
         if (ControlOutput) printf("Deleting shared memory for thread %d\n", index+1);
         shmctl (Connections[index].shmId, IPC_RMID, (struct shmid_ds *)NULL);
         free_shm(NULL,Connections[index].shmId);
      }
   }

   exit (status);
}

/*==============================================================================
 * Function:
 *   transfer_routine - Retrieve data transfer using mover protocol
 *
 * Arguments:
 *   index  - Index into Connections array for this thread
 *
 * Return Values:
 *    <none>
 *============================================================================*/

void transfer_routine (int index)
{
   int                   status, tmp; /* Return, temporary values */
   int                   transferListenSocket; /* Socket listen descriptors */
   int                   transferSocketFd; /* Transfer accept socket */
   struct sockaddr_in    transferSocketAddr; /* Transfer socket address */
   int                   bytesReceived;
   initiator_msg_t       initMessage, initReply;
   initiator_ipaddr_t    ipAddr;   /* TCP socket address info */
   initiator_shmaddr_t   shmAddr;  /* Shared memory address info */
   initiator_san3paddr_t sanAddr;  /* SAN3P address info */
   completion_msg_t      completionMessage;
   char                  *buffer;  /* Transfer data buffer */
   char                  *freeBuffer;  /* Transfer data buffer */

  
   if (ControlOutput)
      printf("Thread %d - Started, using control socket %d\n", index+1,
             Connections[index].controlSocketFd);

   Connections[index].shmId = -1;
   transferListenSocket = transferSocketFd = -1;

   buffer = freeBuffer = NULL;


   /* Loop until we reach a condition to discontinue talking with Mover
    */
   while (TransferStatus == HPSS_E_NOERROR) {

      /* Get the next transfer initiation message from the Mover.
       * HPSS_ECONN will be returned when the Mover is done.
       */
      status = mvrprot_recv_initmsg (Connections[index].controlSocketFd,
                                     &initMessage);

      if (ControlOutput) printf("Thread %d - mvrprot_recv_initsg returned %ld\n", 
                                index+1, status);

      if (status == HPSS_ECONN) {
         break;                    /* break out of the while loop */
      }
      else if (status != HPSS_E_NOERROR) {

         hpss_error ("mvrprot_recv_initmsg returned", status);
         TransferStatus = status;
         continue;
      }

      if (ControlOutput) {
         printf ("Thread %d - Mover ready to send ", index+1);
         print_bytes64 (initMessage.Length);
         printf (" at offset ");
         print_bytes64 (initMessage.Offset);
         printf (" via %s\n",
                 initMessage.Type == NET_ADDRESS ? "TCP" :
                 initMessage.Type == SHM_ADDRESS ? "SHM" : "SAN3P");
      }

      /* Tell the Mover we will send the address next
       */
      initReply.Flags = MVRPROT_COMP_REPLY | MVRPROT_ADDR_FOLLOWS;

      /* Let's agree to use the transfer protocol selected by the Mover and let's 
       * accept the offset.  However, the number of bytes the Mover can transfer
       * at one time is limited by our buffer size, so we tell the Mover how
       * much of the data he has offerred that we are willing to accept.
       */
      initReply.Type   = initMessage.Type;
      initReply.Offset = initMessage.Offset;
    
      if (gt64m (initMessage.Length, cast64m(BufferSize)))
         initReply.Length = cast64m(BufferSize);
      else
         initReply.Length = initMessage.Length;

      /* Send our response back to the Mover
       */
      status = mvrprot_send_initmsg (Connections[index].controlSocketFd,
                                     &initReply);
    
      if (status != HPSS_E_NOERROR) {
         hpss_error ("mvrprot_send_initmsg", status);
         TransferStatus = status;
         continue;
      }

      /* Based on the type of transfer protocol, allocate memory, send address
       * information, and receive the data from the HPSS Mover
       */
      switch (initMessage.Type) {

         case SHM_ADDRESS:

            /* If we have not already created the shared memory segment for this
             * thread, do it now 
             */
            if (!buffer) {

               Connections[index].shmId = shmget (IPC_PRIVATE, BufferSize,
                                                  S_IRWXU | S_IRWXG | S_IRWXO);
               if (Connections[index].shmId == -1) {
                  perror ("shmget");
                  TransferStatus = errno;
                  continue;
               }
    
               buffer = shmat (Connections[index].shmId, NULL, 0);

               if (!buffer) {
                  perror ("shmat");
                  TransferStatus = errno;
                  continue;
               }

               memset (&shmAddr, 0, sizeof(shmAddr));
               shmAddr.Flags             = HPSS_IOD_HOLD_RESOURCES; 
               shmAddr.ShmAddr.ShmID     = Connections[index].shmId;
            }

            /* Tell the Mover what our shared memory address is
             */
            status = mvrprot_send_shmaddr (Connections[index].controlSocketFd,
                                           &shmAddr);

            if (status != HPSS_E_NOERROR) {
               hpss_error ("mvrprot_send_shmaddr", status);
               TransferStatus = status;
               continue;
            }

            /* At this point, the Mover is moving the data into shared memory
             * and we wait for the completion message
             */
            break;
          
         case NET_ADDRESS:

            /* The first time through, allocate the memory buffer
	     * and data transfer socket
             */
            if (!buffer) {
	       if(ShmBuffer) {
                  buffer = malloc_shm(BufferSize, &Connections[index].shmId);
	       }
	       else {
                  buffer = malloc (BufferSize);
                  if (!buffer) {
                     perror ("malloc");
                     TransferStatus = errno;
                     continue;
                  }
               }

               transferListenSocket = socket (AF_INET, SOCK_STREAM, 0);

               if (transferListenSocket == -1) {
                  perror ("socket");
                  TransferStatus = errno;
                  continue;
               }
    
               if (ControlOutput) {
                  printf ("Thread %d - Opened transfer listen socket %d\n", index+1,
                          transferListenSocket);
               }

               (void)memset (&transferSocketAddr, 0, sizeof(transferSocketAddr));
               transferSocketAddr.sin_family      = AF_INET;
               transferSocketAddr.sin_port        = 0;

               /* Select the hostname (IP address) in a round-robin fashion
                */
               pthread_mutex_lock (&GlobalVarMutex);

               transferSocketAddr.sin_addr.s_addr = HostList[CurrentHost++].ipAddr;
               if (CurrentHost == NumHosts) CurrentHost = 0;

               pthread_mutex_unlock (&GlobalVarMutex);

               if (bind (transferListenSocket,
                         (const struct sockaddr*)&transferSocketAddr, 
                         sizeof(transferSocketAddr)) == -1) {
                  perror ("bind");
                  return;
               }

               tmp = sizeof (transferSocketAddr);

               (void)memset (&transferSocketAddr, 0, sizeof(transferSocketAddr));

               if (getsockname (transferListenSocket, 
                                (struct sockaddr *)&transferSocketAddr, 
                                (sockapi_len_t *)&tmp) == -1) {
                  perror ("getsockname");
                  TransferStatus = errno;
                  continue;
               }
    
               if (listen (transferListenSocket, SOMAXCONN) == -1) {
                  perror ("listen");
                  TransferStatus = errno;
                  continue;
               }

               if (VerboseOutput) {
                  printf ("Thread %d - Using TCP network address %d.%d.%d.%d:%d\n", 
                          index+1,
                          (transferSocketAddr.sin_addr.s_addr & 0xff000000) >> 24,
                          (transferSocketAddr.sin_addr.s_addr & 0x00ff0000) >> 16,
                          (transferSocketAddr.sin_addr.s_addr & 0x0000ff00) >> 8,
                          (transferSocketAddr.sin_addr.s_addr & 0x000000ff),
                          transferSocketAddr.sin_port);
               }
        
               memset (&ipAddr,  0, sizeof(ipAddr));
               ipAddr.IpAddr.SockTransferID  = cast64m (RequestId);
               ipAddr.IpAddr.SockAddr.family = transferSocketAddr.sin_family;
               ipAddr.IpAddr.SockAddr.addr   = transferSocketAddr.sin_addr.s_addr;
               ipAddr.IpAddr.SockAddr.port   = transferSocketAddr.sin_port;
               ipAddr.IpAddr.SockOffset      = cast64m (0);
            }

            /* Tell the Mover what socket to send the data to
             */
            status = mvrprot_send_ipaddr (Connections[index].controlSocketFd, &ipAddr);
      
            if (status != HPSS_E_NOERROR) {
               hpss_error ("mvrprot_send_ipaddr", status);
               TransferStatus = status;
               continue;
            }
      
            /* Wait for the new Mover socket connection, if you don't already
             * have one
             */
            if (transferSocketFd == -1) {

               tmp = sizeof(transferSocketAddr);
        
               while ((transferSocketFd = 
                       accept (transferListenSocket, 
                               (struct sockaddr *)&transferSocketAddr, 
                               (sockapi_len_t *)&tmp)) < 0) {
          
                  if ((errno != EINTR) && (errno != EAGAIN)) {
                     TransferStatus = errno;
                     break;
                  }
               } /* end while */

               if (ControlOutput)
                  printf("Thread %d - accept received, new transfer socket is %d\n", 
                         index+1, transferSocketFd);

               if (VerboseOutput) {
                  printf ("Peer Address = "); 
                  print_peer(stdout,transferSocketFd);
                  printf ("\n");
               }

               socket_setoptions (transferSocketFd, NULL);
               if (VerboseOutput)
                  check_sockopts("data socket options", transferSocketFd);

               if (transferSocketFd < 0) continue;

            }

            /* Receive the data from the Mover via our socket
             */
            status = mover_socket_recv_data (transferSocketFd, cast64m (RequestId),
                                             initMessage.Offset, buffer,
                                             low32m (initReply.Length),
                                             &bytesReceived, 1);
            if (status <= 0) {
               hpss_error ("mover_socket_recv_data", status);
               TransferStatus = status;
            }
            break;

         case SAN3P_ADDRESS:

            /* The first time through, allocate the memory buffer
	     * and data transfer socket
             */
	    if(ShmBuffer) {
               buffer = malloc_shm(BufferSize, &Connections[index].shmId);
	    }
	    else {
               if (!buffer) {
                  status = hpss_PAMalloc(BufferSize,&freeBuffer,&buffer);
	          if(status) {
		     freeBuffer = buffer = NULL;
                     hpss_error("hpss_PAMalloc", status);
                     TransferStatus = status;
                     continue;
                  }
               }
            }

            /* Build/send the SAN3P initiator address message */
            sanAddr.Flags = 0;
            sanAddr.San3pAddr.SAN3PTransferID = (unsigned32)RequestId;
            memset(&sanAddr.San3pAddr.SanSecurityToken,0x0,
                  sizeof(san_sec_token_t));

            status = mvrprot_send_san3paddr(Connections[index].controlSocketFd,
					    &sanAddr);
            if (status != HPSS_E_NOERROR) {
               hpss_error ("mvrprot_send_san3paddr", status);
               TransferStatus = status;
               continue;
            }

	    /* Read our data */
            status = san3p_Read((san3paddress_t *)&sanAddr,
                                Connections[index].controlSocketFd,
		                buffer,
                                low32m(initReply.Length),
				(unsigned int *)&bytesReceived);

            /* Send a completion message back to the mover */
            completionMessage.Flags = 0;
            completionMessage.Status = status;
            completionMessage.BytesMoved = cast64m(bytesReceived);
            memcpy((char *)completionMessage.SecurityTicket,
                   (char *)initMessage.SecurityTicket,
                   MVRPROT_TICKET_LENGTH);
            tmp = mvrprot_send_compmsg(Connections[index].controlSocketFd,
                                       &completionMessage);
			   
            if (status != HPSS_E_NOERROR) {
               hpss_error ("san3p_Read", status);
               TransferStatus = status;
               continue;
            }

            if (tmp != HPSS_E_NOERROR) {
               hpss_error ("mvrprot_send_compmsg", status);
               TransferStatus = status;
               continue;
            }

            break;          

         default:
            break;

      } /* end switch */

      if(initMessage.Type == SHM_ADDRESS || initMessage.Type == NET_ADDRESS) {
         /* Get a transfer completion message from the Mover
          */
         status = mvrprot_recv_compmsg (Connections[index].controlSocketFd,
                                        &completionMessage);

         if (status != HPSS_E_NOERROR) {
            hpss_error ("mvrprot_recv_compmsg", status);
            TransferStatus = status;
            continue;
         }  
      }
      
      if (VerboseOutput) {
         printf ("Thread %d - ", index+1);
      
         print_bytes64 (completionMessage.BytesMoved);
         printf(" received at offset ");
         print_bytes64 (initMessage.Offset);
         printf(" via ");
      
         switch (initMessage.Type) {
            case SHM_ADDRESS:
               printf ("SHM\n");
               break;
            case NET_ADDRESS:
               printf ("TCP\n");
               break;
            case SAN3P_ADDRESS:
               printf ("SAN3P\n");
               break;
            default:
               break;
         }
      }

      pthread_mutex_lock (&GlobalVarMutex);
      inc64m (TotalBytesRead, completionMessage.BytesMoved);
      pthread_mutex_unlock (&GlobalVarMutex);
      
   } /* end while loop */

   if (ControlOutput) {
      printf("Closing down thread %d\n", index+1);
   }

   /* Clean up, based on the transfer protocol
    */
   switch (initMessage.Type) {

      case SHM_ADDRESS:

         /* Remove the shared memory segment if it got allocated
          */
         if (Connections[index].shmId != -1) {
            shmdt (buffer);
            shmctl (Connections[index].shmId, IPC_RMID, (struct shmid_ds *)NULL);
            Connections[index].shmId = -1;
         }
         break;

      case SAN3P_ADDRESS:
         /* Free the buffer if it was allocated
          */
	 if(ShmBuffer) { 
            free_shm(buffer,Connections[index].shmId);
	 }
	 else if (buffer && freeBuffer) {
	    (void) free(freeBuffer);
         }
         break;

      case NET_ADDRESS:
      default:

         /* Close down the TCP transfer socket if it got opened
          */
         if (transferSocketFd != -1) {
            (void) close (transferSocketFd);
         }

         /* Free the buffer if it was allocated
          */
	 if(ShmBuffer) { 
            free_shm(buffer,Connections[index].shmId);
	 }
	 else if (buffer) {
	    (void) free(buffer);
         }

         break;

   } /* end switch */

   /* Close the control socket
    */
   (void) close (transferListenSocket);

   /* Close the control socket and mark this connection as not
    * active
    */
   pthread_mutex_lock (&GlobalVarMutex);

   (void) close (Connections[index].controlSocketFd);

   Connections[index].active = 0;

   pthread_mutex_unlock (&GlobalVarMutex);

   return;
}
#endif
