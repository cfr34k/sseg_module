ifneq ($(KERNELRELEASE),)
	obj-m := sseg.o
	sseg-objs := signalling.o sseg_main.o
else
	#KERNELDIR ?= /usr/src/linux-$(shell uname -r)
	KERNELDIR ?= /usr/src/linux
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
