obj-m += frontend.o

all:
	make CC="gcc -g -fno-pic -fno-pie" -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
