SUMMARY = "GPIO LED Controller Application"
DESCRIPTION = "Userspace application to control GPIO LEDs via descriptor driver"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

COMPATIBLE_MACHINE = "raspberrypi0-wifi"

SRC_URI = "file://led_ctrl.c \
           file://led_ctrl.h \
           file://Makefile"

S = "${WORKDIR}"

do_compile() {
    oe_runmake
}

do_install() {
    oe_runmake install DESTDIR=${D}
}

RDEPENDS:${PN} = "kernel-module-gpio-led-driver"
FILES:${PN} = "/usr/bin/led_ctrl"