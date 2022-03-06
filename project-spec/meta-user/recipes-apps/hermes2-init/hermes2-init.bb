#
# This file is the hermes2-init recipe.
#

SUMMARY = "hermes2-init script"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
 
SRC_URI = "file://hermes2-init \
	"
SRC_URI += "file://gselect.sh"


S = "${WORKDIR}"

FILESEXTRAPATHS_prepend	:= "${THISDIR}/files:"

inherit update-rc.d

INITSCRIPT_NAME = "hermes2-init"
INITSCRIPT_PARAMS = "start 99 5 . stop 20 0 1 6 ."

do_install() {
	     install -d ${D}/${sysconfdir}/init.d
	     install -m 0755 ${S}/hermes2-init ${D}${sysconfdir}/init.d/hermes2-init
	     install -d ${D}/home/root
	     install -m 0755 ${S}/gselect.sh 	${D}/home/root/gselect.sh
}
FILES_${PN} += "${sysconfdir}/*"
FILES_${PN} += " /home/root/*"