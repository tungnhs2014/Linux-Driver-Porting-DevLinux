obj-$(CONFIG_GPIO_LED_DESCRIPTOR_DRIVER) += gpio_led_driver.o

ifneq ($(KERNELRELEASE),)
    obj-m := gpio_led_driver.o
else
    KDIR ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

.PHONY: default clean install
endif