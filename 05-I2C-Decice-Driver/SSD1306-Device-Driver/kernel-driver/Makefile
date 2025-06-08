# SSD1306 OLED Driver Makefile
obj-$(CONFIG_SSD1306_OLED_DRIVER) += ssd1306_driver.o

ifneq ($(KERNELRELEASE),)
    obj-m := ssd1306_driver.o
else
    KDIR ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: default clean
endif