#
# This file is the openocd recipe.
#

SUMMARY = "openocd application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://openocd               \
           file://h2-jtag.cfg           \
           file://h2-swd.cfg            \
           file://innoh2.cfg            \
          "

S = "${WORKDIR}"

do_install() {
	     install -d ${D}${bindir}
	     install -d ${D}${datadir}/openocd/scripts
	     install -m 0755 openocd ${D}${bindir}
	     install -m 0755 h2-jtag.cfg ${D}${datadir}/openocd/scripts
	     install -m 0755 h2-swd.cfg ${D}${datadir}/openocd/scripts
	     install -m 0755 innoh2.cfg ${D}${datadir}/openocd/scripts
}
