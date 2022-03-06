#
# This file is the Inter-Procedure Call recipe.
#

SUMMARY = "PACE Inter-Procedure Call library"
SECTION = "PETALINUX/libs"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
	file://Makefile \
	file://ipc_base.h \
	file://ipc_ret.h \
	file://ipclib.c \
	file://ipclib.h \
	file://protocol.c \
	file://protocol.h \
	file://socket.c \
	file://socket.h \
	file://threads.c \
	file://threads.h \
	"

S = "${WORKDIR}"

do_compile() {
	oe_runmake
}

do_install() {
	install -d ${D}${libdir}
	oe_libinstall -so libpaceipc ${D}${libdir}

	install -d ${D}${includedir}/paceipc/
	install -m 0644 ${S}/ipclib.h ${D}${includedir}/paceipc/
}