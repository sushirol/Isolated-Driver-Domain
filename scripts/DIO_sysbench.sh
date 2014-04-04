filename="results.txt"
touch $filename
req=0
for i in 1 2 3 4 5 6 7 8 9 10 16 24 32 
do
  req=$(expr $i \* 10000 )
  echo $req
    echo "sysbench --num-threads=$i --test=fileio --file-total-size=1G --max-requests=$req --file-extra-flags=direct --file-test-mode=$1 run" >> $filename
    echo "
    " >> $filename
    sysbench --num-threads=$i --test=fileio --file-total-size=1G --max-requests=$req --file-extra-flags=direct --file-test-mode=$1 run >> $filename
    echo "=======================================================
    " >> $filename
done
