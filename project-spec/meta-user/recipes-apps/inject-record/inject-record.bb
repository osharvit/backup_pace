#
# This file is the inject-record recipe.
#

SUMMARY = "Simple inject-record application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
DEPENDS += "dma-proxy"
DEPENDS += "libpaceipc"
DEPENDS += "pacecli"
CFLAGS_prepend = "-I ${D}${includedir}"
SRC_URI = "file://inject-record.c \
	   file://Makefile \
		  "

S = "${WORKDIR}"
do_compile() {
#	     oe_runmake
	${CC} ${CFLAGS} ${LDFLAGS} -o inject-record inject-record.c -lpaceipc -lpthread
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 inject-record ${D}${bindir}
}
