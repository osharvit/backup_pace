#
# This file is the aximon recipe.
#

SUMMARY = "aximon application for axi performance monitor"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://main.c \
	"
SRC_URI +="file://xaxipmon.c" 
SRC_URI +="file://xaxipmon.h"
SRC_URI +="file://Makefile"

S = "${WORKDIR}"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 ${S}/aximon ${D}${bindir}
}
