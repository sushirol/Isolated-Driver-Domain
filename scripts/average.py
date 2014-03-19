#!/usr/bin/python
import sys
filename = sys.argv[1]
filename_1 = filename + '-1.xls'
filename_2 = filename + '-2.xls'
filename_3 = filename + '-3.xls'
filename_4 = filename + '-4.xls'
filename_5 = filename + '-5.xls'
filename_6 = filename + '-6.xls'
filename_7 = filename + '-7.xls'
filename_8 = filename + '-8.xls'
filename_9 = filename + '-9.xls'
filename_10 = filename + '-10.xls'
filename_av = filename + '-av.xls'
f_1 = open(filename_1,"r")
f_2 = open(filename_2,"r")
f_3 = open(filename_3,"r")
f_4 = open(filename_4,"r")
f_5 = open(filename_5,"r")
f_6 = open(filename_6,"r")
f_7 = open(filename_7,"r")
f_8 = open(filename_8,"r")
f_9 = open(filename_9,"r")
f_10 = open(filename_10,"r")
f_av = open(filename_av,"w")
lines_1 = f_1.readlines()
lines_2 = f_2.readlines()
lines_3 = f_3.readlines()
lines_4 = f_4.readlines()
lines_5 = f_5.readlines()
lines_6 = f_6.readlines()
lines_7 = f_7.readlines()
lines_8 = f_8.readlines()
lines_9 = f_9.readlines()
lines_10 = f_10.readlines()

a_0 = float(lines_1[0]) + float(lines_2[0]) + float(lines_3[0]) + float(lines_4[0]) + float(lines_5[0]) + float(lines_6[0]) + float(lines_7[0]) + float(lines_8[0]) + float(lines_9[0]) + float(lines_10[0])
a_1 = float(lines_1[1]) + float(lines_2[1]) + float(lines_3[1]) + float(lines_4[1]) + float(lines_5[1]) + float(lines_6[1]) + float(lines_7[1]) + float(lines_8[1]) + float(lines_9[1]) + float(lines_10[1])
a_2 = float(lines_1[2]) + float(lines_2[2]) + float(lines_3[2]) + float(lines_4[2]) + float(lines_5[2]) + float(lines_6[2]) + float(lines_7[2]) + float(lines_8[2]) + float(lines_9[2]) + float(lines_10[2])
a_3 = float(lines_1[3]) + float(lines_2[3]) + float(lines_3[3]) + float(lines_4[3]) + float(lines_5[3]) + float(lines_6[3]) + float(lines_7[3]) + float(lines_8[3]) + float(lines_9[3]) + float(lines_10[3])
a_4 = float(lines_1[4]) + float(lines_2[4]) + float(lines_3[4]) + float(lines_4[4]) + float(lines_5[4]) + float(lines_6[4]) + float(lines_7[4]) + float(lines_8[4]) + float(lines_9[4]) + float(lines_10[4])
a_5 = float(lines_1[5]) + float(lines_2[5]) + float(lines_3[5]) + float(lines_4[5]) + float(lines_5[5]) + float(lines_6[5]) + float(lines_7[5]) + float(lines_8[5]) + float(lines_9[5]) + float(lines_10[5])
a_6 = float(lines_1[6]) + float(lines_2[6]) + float(lines_3[6]) + float(lines_4[6]) + float(lines_5[6]) + float(lines_6[6]) + float(lines_7[6]) + float(lines_8[6]) + float(lines_9[6]) + float(lines_10[6])
a_7 = float(lines_1[7]) + float(lines_2[7]) + float(lines_3[7]) + float(lines_4[7]) + float(lines_5[7]) + float(lines_6[7]) + float(lines_7[7]) + float(lines_8[7]) + float(lines_9[7]) + float(lines_10[7])
a_8 = float(lines_1[8]) + float(lines_2[8]) + float(lines_3[8]) + float(lines_4[8]) + float(lines_5[8]) + float(lines_6[8]) + float(lines_7[8]) + float(lines_8[8]) + float(lines_9[8]) + float(lines_10[8])
a_9 = float(lines_1[9]) + float(lines_2[9]) + float(lines_3[9]) + float(lines_4[9]) + float(lines_5[9]) + float(lines_6[9]) + float(lines_7[9]) + float(lines_8[9]) + float(lines_9[9]) + float(lines_10[9])

a_0 = a_0/10;
a_1 = a_1/10;
a_2 = a_2/10;
a_3 = a_3/10;
a_4 = a_4/10;
a_5 = a_5/10;
a_6 = a_6/10;
a_7 = a_7/10;
a_8 = a_8/10;
a_9 = a_9/10;

f_1.close()
f_2.close()
f_3.close()
f_4.close()
f_5.close()
f_6.close()
f_7.close()
f_8.close()
f_9.close()
f_10.close()

f_av.write('1 ')
f_av.write(ascii(a_0))
f_av.write("\n")
f_av.write('2 ')
f_av.write(ascii(a_1))
f_av.write("\n")
f_av.write('3 ')
f_av.write(ascii(a_2))
f_av.write("\n")
f_av.write('4 ')
f_av.write(ascii(a_3))
f_av.write("\n")
f_av.write('5 ')
f_av.write(ascii(a_4))
f_av.write("\n")
f_av.write('6 ')
f_av.write(ascii(a_5))
f_av.write("\n")
f_av.write('7 ')
f_av.write(ascii(a_6))
f_av.write("\n")
f_av.write('8 ')
f_av.write(ascii(a_7))
f_av.write("\n")
f_av.write('9 ')
f_av.write(ascii(a_8))
f_av.write("\n")
f_av.write('10 ')
f_av.write(ascii(a_9))
f_av.write("\n")

print(a_1)
