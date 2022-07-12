MODULE=arp-guard
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
	sudo insmod arp-guard.ko

remove:
	sudo rmmod arp-guard.ko
	
clean:
	make -C  ${KDIR} M=${PWD} clean