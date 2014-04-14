cat $1 | grep -A 1 "CPU    %usr" | grep "all" > /tmp/$1
python2.7 minimum.py /tmp/$1 
