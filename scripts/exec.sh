mapfile < /root/scripts/config
temp=${MAPFILE[0]}
op=`echo "$temp" | sed -E 's/(\S*)(\s)/[\1]\2\n/g'`
sh DIO_prepare.sh $op 
sh file_cache.sh
for i in 1 2 3 4 5 6 7 8 9 10
do
  rm results.txt
  rm cpu.txt
  filename="/root/results/$op-ramdisk-$i.txt"
  cpufile="/root/cpu/$op-ramdisk-$i.cpu"
  touch $filename
  touch $cpufile
  sh /mnt/DIO_sysbench.sh $op
  cat results.txt > $filename
  cat cpu.txt > $cpufile
done
sh DIO_cleanup.sh $op 
