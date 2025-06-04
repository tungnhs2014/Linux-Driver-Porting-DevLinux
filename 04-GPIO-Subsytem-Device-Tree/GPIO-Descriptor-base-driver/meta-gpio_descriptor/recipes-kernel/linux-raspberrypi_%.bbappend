FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-GPIO-LED-descriptor-driver.patch"

KERNEL_MODULE_AUTOLOAD += "gpio_led_driver"
