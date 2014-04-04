#!/usr/bin/env python
import sys
fn = sys.argv[1]
filenameList = []
outputList = []
timeList = []
for i in range(1, 11):
    filenameList.append(fn + '-' + `i` + '.txt')
    outputList.append('cpu-' + fn + '-' + `i` + '.cpu')
    timeList.append(fn + '-' + `i` + '.time')

#print filenameList
#print outputList

for i in range (0,10):
    stat_fd = open(filenameList[i],"r")
    time_fd = open(timeList[i],"r")
    line = stat_fd.readlines()
    time_line = time_fd.readlines()
    out_fd = open(outputList[i],"w")

    for x in range(0, 10):
        timex = `time_line[x]`
        timex = timex[:-4]
        timex = timex[1:]

        field_l_1 = line[(x*3)+1].split()
        field_l_2 = line[(x*3)+2].split()

        cpu_l_1 = float(field_l_1[1]) + float(field_l_1[2]) + float(field_l_1[3]) + float(field_l_1[4])
        cpu_l_2 = float(field_l_2[1]) + float(field_l_2[2]) + float(field_l_2[3]) + float(field_l_2[4])
        cpu = float(cpu_l_2) - float(cpu_l_1)

        cpu = cpu / float(timex)
        out_fd.write(`cpu`)
        out_fd.write("\n")
#        print cpu
    stat_fd.close()
    out_fd.close()
