filename="results.txt"
cpufile="cpu.txt"
touch $cpuname
touch $filename
req=0
for i in 1 2 3 4 5 6 7 8 9 10 16 24 32 
do
  req=$(expr $i \* 100000 )
  echo "sysbench --num-threads=$i --test=fileio --file-total-size=1G --max-requests=$req --file-extra-flags=direct --file-test-mode=$1 run" >> $filename
  echo -e "\n" >> $filename
  echo "thread $i" > $cpufile
  cat /proc/stat | grep "cpu " > $cpufile
  sysbench --num-threads=$i --test=fileio --file-total-size=1G --max-requests=$req --file-extra-flags=direct --file-test-mode=$1 run >> $filename
  cat /proc/stat | grep "cpu " > $cpufile
  echo -e "=======================================================\n" >> $filename
done
