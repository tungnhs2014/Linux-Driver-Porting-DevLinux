SUMMARY = "LED Control Application"
DESCRIPTION = "Userspace application to control LED via character device"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://led_control.c \
           file://led_control.h \
           file://Makefile"

S = "${WORKDIR}"

do_compile() {
    oe_runmake
}

do_install() {
    oe_runmake install DESTDIR=${D}
}

# Depend on kernel module
RDEPENDS_${PN} = "kernel-module-led-driver"