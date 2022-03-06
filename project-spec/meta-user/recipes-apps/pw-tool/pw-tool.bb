#
# This file is the pw-tool recipe.
#

SUMMARY = "Simple pw-tool application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://pw-tool.c \
	   file://axis-fifo-eth-loop.c \
	   file://axis-fifo.h \
	   file://protocol.c \
	   file://protocol.h \
	   file://common.h \
	   file://Makefile \
		  "

S = "${WORKDIR}"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 pw-tool ${D}${bindir}
}
