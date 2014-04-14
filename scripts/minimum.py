import sys
filename = sys.argv[1]

f = open(filename, "r")
var = 100
for line in f:
	temp = line.split()
	var1 = float(temp[11])
	if var1 < var:
		var = var1
print(var)
