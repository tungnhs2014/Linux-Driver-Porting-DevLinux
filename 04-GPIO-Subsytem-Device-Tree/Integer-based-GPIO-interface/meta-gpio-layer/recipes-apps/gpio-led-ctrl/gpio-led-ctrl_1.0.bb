SUMMARY = "GPIO LED Control Application (Legacy GPIO Interface)"
DESCRIPTION = "Userspace application to control LED via legacy GPIO interface"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://gpio_led_ctrl.c \
           file://gpio_led_ctrl.h \
           file://Makefile"

S = "${WORKDIR}"

do_compile() {
    oe_runmake
}

do_install() {
    oe_runmake install DESTDIR=${D}
}

# Skip LDFLAGS QA check
INSANE_SKIP_${PN} = "ldflags"

# Depend on kernel module
RDEPENDS_${PN} = "kernel-module-gpio-led-driver"