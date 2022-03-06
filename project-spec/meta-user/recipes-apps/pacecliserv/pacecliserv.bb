#
# This file is the PACE CLI service(daemon) recipe.
#

SUMMARY = "PACE CLI daemon"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
	file://Makefile      \
	file://client.cpp    \
	file://client.h      \
	file://clientmgr.cpp \
	file://clientmgr.h   \
	file://connmgr.cpp   \
	file://connmgr.h     \
	file://gen-types.h   \
	file://h2emul.cpp    \
	file://h2emul.h      \
	file://main.cpp      \
	file://retcodes.h    \
	file://socket.cpp    \
	file://socket.h      \
	"

S = "${WORKDIR}"

do_compile() {
        oe_runmake
}

do_install() {
        install -d ${D}${bindir}
        install -m 0755 ${S}/pacecliserv ${D}${bindir}

}