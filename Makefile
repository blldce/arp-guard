MODULE=arpguard
PWD := $(shell pwd)
KERNELRELEASE := $(shell uname -r)
KDIR := /lib/modules/${KERNELRELEASE}/build
MDIR := /lib/modules/${KERNELRELEASE}
obj-m := ${MODULE}.o
${MODULE}-objs := ${MODULE}.o dhcp.o

all:
	make -C ${KDIR} M=${PWD} modules
	rm -rf *.mod.c .*.cmd *.symvers *.o

install:
	sudo insmod arpguard.ko

remove:
	sudo rmmod arpguard.ko
	
clean:
	make -C  ${KDIR} M=${PWD} clean