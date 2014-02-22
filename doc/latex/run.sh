# !/bin/bash


function check() {
if [ $? -ne 0 ] 
then
	#clear
        echo "building" $1.tex "FAIL" 
	rm -f $1.pdf  $1.aux $1.bbl $1.blg $1.log $1.out $1.dvi
        exit
        #else
        #               echo "PASS"
fi
}


function with_bibilography () {
	rm -f $1.pdf  $1.aux $1.bbl $1.blg $1.log $1.out $1.dvi
	latex  $1
	bibtex $1
	latex  $1
	latex  $1
	check  $1
	rm -f $1.pdf $1.bbl $1.blg $1.log $1.out
	#xdvi -s 6 $1.dvi
	dvipdfmx $1.dvi
	check $1
	rm -f $1.aux $1.dvi
        #cp $1.pdf ~/
	#evince $1.pdf
	#acroread $1.pdf &
}

function without_bibilography () {
	rm -f $1.pdf  $1.aux $1.bbl $1.blg $1.log $1.out $1.dvi
	latex  $1
	check  $1
	rm -f $1.pdf  $1.bbl $1.blg $1.log $1.out
	#xdvi -s 6 $1.dvi
	dvipdfmx $1.dvi    
	check $1
	rm -f $1.aux $1.dvi
	#cp $1.pdf ~/
	#evince $1.pdf &
	#acroread $1.pdf
} 


with_bibilography $1
