FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-SSD1306-OLED-I2C-driver.patch"

KERNEL_MODULE_AUTOLOAD += "gpio_led_driver"
