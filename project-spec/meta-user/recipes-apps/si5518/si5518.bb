#
# This file is the si5518 control application recipe.
#

SUMMARY = "si5518 application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = 	"file://cmdmgr.cpp	\
		file://cmdmgr.h		\
		file://file.cpp		\
		file://file.h		\
		file://general.h	\
		file://main.cpp		\
		file://rc.h		\
		file://spidev.h		\
		file://spimgr.cpp	\
		file://spimgr.h		\
		file://Makefile \
		"

S = "${WORKDIR}"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 si5518 ${D}${bindir}
}
