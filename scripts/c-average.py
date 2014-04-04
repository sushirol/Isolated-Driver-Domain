#!/usr/bin/python
import sys
filename = sys.argv[1]

filenameList = []
outputList = []
fList=[]
a = []

for i in range(1, 11):
    filenameList.append(filename + '-' + `i` + '.cal')

for i in range(0, 10):
    fList.append(open(filenameList[i],"r"))

f_av = open(filename + '-av.data',"w")

for j in range(0,13):
    temp = 0
    for i in range(0, 10):
        temp = float(fList[i].readline()) + temp
    temp = temp/10
    a.append(temp)

for i in range(0, 13):
    f_av.write(`i+1`)
    f_av.write(" ")
    f_av.write(`a[i]`)
    f_av.write("\n")
    print a[i]

for i in range(0, 10):
    fList[i].close()
f_av.close()
