obj-m += backend.o

all:
	make CC="gcc -Werror -fno-pic -fno-pie" -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
