MODULE := test_iio

ifneq ($(KERNELRELEASE),)
obj-m += $(MODULE).o
$(MODULE)-y := test_iio_core.o test_iio_buffer.o
else
include ../../kernel_module.mk
endif
