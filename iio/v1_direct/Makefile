MODULE := test_iio

ifneq ($(KERNELRELEASE),)
obj-m += $(MODULE).o
else
include ../../kernel_module.mk
endif
