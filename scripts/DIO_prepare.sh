sysbench --num-threads=16 --test=fileio --file-extra-flags=direct --file-total-size=1G --file-test-mode=$1 prepare
echo 0 > /proc/sys/vm/dirty_writeback_centisecs
