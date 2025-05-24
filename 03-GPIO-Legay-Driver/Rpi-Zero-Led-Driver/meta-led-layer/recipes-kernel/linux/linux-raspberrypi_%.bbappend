FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

# Apply LED driver patch
SRC_URI += "file://0001-Add-LED-character-device-driver-for-Pi-Zero-W.patch"

# Auto-load module at boot
KERNEL_MODULE_AUTOLOAD += "led_driver"
