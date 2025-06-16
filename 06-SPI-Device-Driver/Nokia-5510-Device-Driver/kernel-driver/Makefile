# Nokia 5110 LCD Driver Makefile
obj-$(CONFIG_NOKIA5110_LCD_DRIVER) += nokia5110_driver.o

ifneq ($(KERNELRELEASE),)
    obj-m := nokia5110_driver.o
else
    KDIR ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: default clean
endif