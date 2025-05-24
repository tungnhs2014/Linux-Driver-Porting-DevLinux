FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

# Apply GPIO legacy driver patch
SRC_URI += "file://0001-Add-GPIO-legacy-driver-for-Pi-Zero-W.patch"

# Auto-load module at boot
KERNEL_MODULE_AUTOLOAD += "gpio_led_driver"