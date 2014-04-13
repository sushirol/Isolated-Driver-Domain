#!/bin/bash

if [ "$1" == "" ]
then
	echo "Please pass the include file."
	exit 1
fi
if [ "$1" == "-h" ]
then
	echo "$0 <templatefile> [preview]"
	echo "$0 -h"
	exit 0
fi

. ./$1

# Calculate timestamp
TS=`date +%m_%d__%I_%M_%S`
MLOG_DIR=$MLOG_DIR"_"$TS
XDD_LOGFILE=""
MPSTAT_LOGFILE=""

mkdir -p $MLOG_DIR
ls $MLOG_DIR

PREVIEW=""
if [ "$2" == "preview" ]
then
	PREVIEW=1
	PASS_DELAY=1
fi

calc_files()
{
	i=0
	TARGET_FILES=""
	LOG_DIR=""

	TARGET_FILES="$TARGET_DEVS "

	LOG_DIR=$MLOG_DIR"/run_t"$TARGET_COUNT"_b"$BS"_"$FILE_SIZE$FILE_SIZE_TYPE$DIRECT_IO
	if [ "$PREVIEW" == "" ]
	then
		mkdir -p $LOG_DIR
	fi
	MPSTAT_LOGFILE="$LOG_DIR/mpstat"
	XDD_LOGFILE="$LOG_DIR/xdd"
	CMD_LOGFILE="$LOG_DIR/xdd_command"
}

run_mpstat()
{
	# Redirect mpstat to log file.
	if [ "$PREVIEW" == "" ]
	then
		$MPSTAT_EXE -A -I SUM 1 0 2>&1> $MPSTAT_LOGFILE &
		sleep 1
	else
		echo "$MPSTAT_EXE -A -I SUM 2 0 2>&1> $MPSTAT_LOGFILE &"
	fi
}

kill_mpstat()
{
	pkill mpstat
}

run_xdd()
{
	echo "############## MARKER ##############" > /var/log/messages
	calc_files

	echo "################################################################   STARTING RUN   ###########################################################################"
	# Start MPSTAT
	run_mpstat

	# Run XDD
	if [ "$PREVIEW" == "" ]
	then
		echo "Running XDD using following.."
		echo "$XDD_EXE $OPERATION -targets $TARGET_COUNT $TARGET_FILES -queuedepth $QDEPTH -blocksize $BS $FILE_SIZE_TYPE $FILE_SIZE -passes $PASS_COUNT $CREATE_NEW $VERBOSE $DATA_PATTERN $DIRECT_IO" | tee $CMD_LOGFILE
		bash $CMD_LOGFILE 2>&1 > $XDD_LOGFILE
		if [ "$?" != "0" ]
		then
			echo "XDD Has failed for some reason... EXITING!!!!!!!!"
			kill mpstat
			exit 1
		fi
	else
		echo "$XDD_EXE $OPERATION -targets $TARGET_COUNT $TARGET_FILES -queuedepth $QDEPTH -blocksize $BS $FILE_SIZE_TYPE $FILE_SIZE -passes 1 $CREATE_NEW $VERBOSE $DATA_PATTERN $DIRECT_IO" 
	fi
	echo "Logs at $LOG_DIR  .."

	# Kill MPSTAT
	kill_mpstat

	# copy logs
	echo "################################################################   ENDING   RUN   ###########################################################################"
}

for BS in $BLOCK_SIZES
do
		run_xdd

		echo "Sleeping for ....... $PASS_DELAY"
		sleep $PASS_DELAY
		if [ "$PREVIEW" == "" ]
		then
			cp /var/log/messages $LOG_DIR/var_log_messages
		fi
done
