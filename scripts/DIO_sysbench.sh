filename="results.txt"
touch $filename
for i in 1 2 3 4 5 6 7 8 9 10
do
    echo "sysbench --num-threads=$i --test=fileio --file-total-size=1G --max-requests=100000 --file-extra-flags=direct --file-test-mode=$1 run" >> $filename
    echo "
    " >> $filename
    sysbench --num-threads=$i --test=fileio --file-total-size=1G --max-requests=100000 --file-extra-flags=direct --file-test-mode=$1 run >> $filename
    echo "=======================================================
    " >> $filename
done
