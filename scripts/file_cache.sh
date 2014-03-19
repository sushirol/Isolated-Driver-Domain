#!/bin/sh

for i in `ls test_file.*`
do
        sudo dd if=./$i of=/dev/null
done
