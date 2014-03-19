mapfile < /root/scripts/config
temp=${MAPFILE[0]}
op=`echo "$temp" | sed -E 's/(\S*)(\s)/[\1]\2\n/g'`
sh DIO_prepare.sh $op 
sh file_cache.sh
for i in 1 2 3 4 5 6 7 8 9 10
do
  filename="/root/results/$op-ramdisk-$i.txt"
  touch $filename
  sh /mnt/DIO_sysbench.sh $op 
  cat results.txt > $filename
  rm results.txt
done
sh DIO_cleanup.sh $op 
