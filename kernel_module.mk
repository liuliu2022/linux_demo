ifndef KDIR
$(error KDIR is not set. Run: source ../kernel_env.sh)
endif

ifndef ARCH
$(error ARCH is not set. Run: source ../kernel_env.sh)
endif

ifndef CROSS_COMPILE
$(error CROSS_COMPILE is not set. Run: source ../kernel_env.sh)
endif

ifndef MODULE
$(error MODULE is not set in the module Makefile)
endif

PWD := $(shell pwd)

.PHONY: all clean rebuild info check deploy

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

rebuild: clean all

info:
	@echo "ARCH          = $(ARCH)"
	@echo "CROSS_COMPILE = $(CROSS_COMPILE)"
	@echo "KDIR          = $(KDIR)"
	@echo "MODULE         = $(MODULE)"
	@echo "MODULE_DIR     = $(PWD)"
	@echo
	@$(CROSS_COMPILE)gcc --version | head -n 1

check: all
	@echo "===== Architecture ====="
	@file $(MODULE).ko
	@echo
	@echo "===== Module Info ====="
	@modinfo $(MODULE).ko

BOARD_USER ?= root
BOARD_DIR  ?= /tmp

deploy: check
ifndef BOARD_IP
	$(error BOARD_IP is not set. Example: make deploy BOARD_IP=192.168.1.100)
endif
	scp $(MODULE).ko $(BOARD_USER)@$(BOARD_IP):$(BOARD_DIR)/
