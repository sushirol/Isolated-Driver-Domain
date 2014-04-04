#!/usr/bin/env python
import sys
fn = sys.argv[1]
filenameList = []
outputList = []
for i in range(1, 11):
    print i
    filenameList.append(fn + '-' + `i` + '.txt')
    outputList.append('cpu-' + fn + '-' + `i` + '.cpu')

print filenameList
print outputList

for i in range (0,10):
    stat_fd = open(filenameList[i],"r")
    line = stat_fd.readlines()
    out_fd = open(outputList[i],"w")

    for x in range(0, 10):
        field_l_1 = line[(x*3)+1].split()
        field_l_2 = line[(x*3)+2].split()
        cpu_l_1 = float(field_l_1[1]) + float(field_l_1[2]) + float(field_l_1[3]) + float(field_l_1[4])
        cpu_l_2 = float(field_l_2[1]) + float(field_l_2[2]) + float(field_l_2[3]) + float(field_l_2[4])
        cpu = float(cpu_l_2) - float(cpu_l_1)
        out_fd.write(`cpu`)
        out_fd.write("\n")
#        print cpu
    stat_fd.close()
    out_fd.close()
