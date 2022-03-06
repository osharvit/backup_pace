#
# This file is the example of PACE CLI Pluging application recipe.
#

SUMMARY = "Simple example of CLI plugin application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://main.c \
           file://Makefile \
		"

S = "${WORKDIR}"

DEPENDS = "libpaceipc"
DEPENDS += "pacecli"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 pacecliplug ${D}${bindir}
}
