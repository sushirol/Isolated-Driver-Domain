#!/bin/bash

EXIT_VAL=0

XDD_TAR="xdd65.013007.tgz"
XDD_DIR="xdd65.013007"

SYSTAT_TAR="sysstat-8.1.5_modified.tar"
SYSTAT_DIR="sysstat-8.1.5"

INSTALL_DIR="./install"
LOG_DIR="./logs"

XDD_COMP_LOG="$PWD/logs/xdd_comp_log"
SYSTAT_COMP_LOG="$PWD/logs/sys_comp_log"

mkdir -p $INSTALL_DIR
mkdir -p $LOG_DIR

fail()
{
	EXIT_VAL=$1
	echo "FAILED::: "$2
}

success()
{
	EXIT_VAL=0
	echo "SUCCESS::: "$1
}

check_and_exit()
{
	if [ "$EXIT_VAL" != "0" ]
	then
		exit $EXIT_VAL
	fi
}

if [ "$1" == "-u" ]
then
	rm -rf ./$INSTALL_DIR || fail 1 "Couldnt cleanup"
	check_and_exit
	success "Cleaned up $INSTALL_DIR"
	rm -rf ./$LOG_DIR || fail 1 "Couldnt cleanup"
	check_and_exit
	success "Cleaned up $LOG_DIR"
	exit 0
fi

# Check XDD
if [ ! -e $XDD_TAR ]
then
	fail 1 "Couldn't find XDD TARBALL"
fi
check_and_exit

success "XDD TARBALL ok"

# Untar and compile xdd
echo "Compiling XDD"
(
	tar xvfpz $XDD_TAR 2>&1 > $XDD_COMP_LOG || fail 1 "Couldnt Untar XDD"
	check_and_exit

	mv $XDD_DIR $INSTALL_DIR
	cd $INSTALL_DIR/$XDD_DIR
	rm -rf ./bin/*

	make -f linux.makefile 2>/dev/null >> $XDD_COMP_LOG || fail 1 "Couldnt compile XDD"
	check_and_exit
	success "Compiled XDD...."
)
check_and_exit


# Check SYSSTAT
if [ ! -e $SYSTAT_TAR ]
then
	fail 1 "Couldn't find SYSSTAT TARBALL"
fi
check_and_exit

success "SYSSTAT TARBALL ok"

echo "Compiling SYSTAT...."
# Untar and compile systat
(
	tar xvfp $SYSTAT_TAR 2>&1 > $SYSTAT_COMP_LOG || fail 1 "Couldnt Untar SYSSTAT"
	check_and_exit

	mv $SYSTAT_DIR $INSTALL_DIR
	cd $INSTALL_DIR/$SYSTAT_DIR
	rm -rf ./bin/*

	make clean 2>&1 > /dev/null
	make 2>&1 >> $SYSTAT_COMP_LOG || fail 1 "Couldnt compile SYSTAT"
	check_and_exit

	success "Compiled SYSTAT"
)
check_and_exit

[ -e $INSTALL_DIR/$XDD_DIR/bin/xdd.linux -a -e $INSTALL_DIR/$SYSTAT_DIR/mpstat ] || fail 1 "Something went wrong"
check_and_exit

success "TOOLKIT Setup successful"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ !!! IMPORTANT !!! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "Add the following to your .bashrc"
echo "export PATH=$PWD/$INSTALL_DIR/$XDD_DIR/bin/:$PWD/$INSTALL_DIR/$SYSTAT_DIR/:\$PATH"
echo ""
echo ""
echo "To run xdd and mpstat"
./run_xdd.sh -h
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ !!! IMPORTANT !!! @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
