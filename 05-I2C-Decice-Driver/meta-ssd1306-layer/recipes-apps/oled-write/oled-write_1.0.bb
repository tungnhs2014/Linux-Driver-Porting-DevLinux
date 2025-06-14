SUMMARY = "SSD1306 OLED Display Controller Application"
DESCRIPTION = "Application to control SSD1306 OLED display via character device"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

COMPATIBLE_MACHINE = "raspberrypi0-wifi"

SRC_URI = "file://oled_write.c \
           file://oled_write.h \
           file://Makefile"

S = "${WORKDIR}"

do_compile() {
    oe_runmake
}

do_install() {
    oe_runmake install DESTDIR=${D}
}

# Runtime dependency on kernel module
RDEPENDS:${PN} = "kernel-module-ssd1306-driver"

FILES:${PN} = "/usr/bin/oled_write"