NAME=`echo "$1" | cut -d'.' -f1`
EXTENSION=`echo "$NAME" | cut -d'.' -f2`
grep "Requests/sec executed" $1 | sed 's/^ *//' > $NAME.tmp
cut -d " " -f 1 $NAME.tmp > $NAME.xls
rm $NAME.tmp
