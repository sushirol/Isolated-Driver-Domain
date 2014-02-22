
# usage: perl TightBoundingBox.pl <file.eps>

# calculates a tight bounding box (using the 'gs' command) and sets that bounding box into <file.eps>.  This is useful where file.eps was created from Visio by printing to the Adobe Generic Postscript Printer into <file.eps>

$inputFileName=$ARGV[0];

# first, find bbox dimensions
$bbox="";
#-sOutputFile=temp_file.bbox
open(GS,"gs -dNOPAUSE -dBATCH -sDEVICE=bbox -sOutputFile=- $inputFileName 2>&1 |");
while (<GS>) {
	chop;
	if (/^\%\%BoundingBox: (.*)/) {
		$bbox=$1;
	}
}
close(GS);
#print $bbox;

# now substitute this into file
$tempFileName="temp_file.eps";
system "cat $inputFileName | tr -d \"\r\" > $tempFileName";
open(IN,"$tempFileName");
#open(OUT,">t2.eps"); # don't clobber the input, just for testing
open(OUT,">$inputFileName");
while (<IN>) {
	chop;
	
	$gotBbox=0;
	if (/^\%\%(.*)BoundingBox: .*/) {
		print OUT "%%"."$1BoundingBox: $bbox\n";
	}
	else {
		print OUT "$_\n";
	}
}
close(IN);
close(OUT);
