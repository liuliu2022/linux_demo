obj-m += test_iio.o

KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH ?=
CROSS_COMPILE ?=

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) \
		M=$(PWD) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		modules

clean:
	$(MAKE) -C $(KDIR) \
		M=$(PWD) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		clean
