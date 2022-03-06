#
# This file is the tca recipe.
#

SUMMARY = "Configuration of IO expanders"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://tca.sh \
		  "
S = "${WORKDIR}"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
#inherit update-rc.d
#INITSCRIPT_NAME = "tca.sh"
#INITSCRIPT_PARAMS = "start 99 S ."
#do_install() {
# install -d ${D}${sysconfdir}/init.d
# install -m 0755 ${S}/tca.sh ${D}${sysconfdir}/init.d/tca.sh
#}
do_install() {
         install -d ${D}${bindir}
         install -m 0755 tca.sh ${D}${bindir}
}

FILES_${PN} += "${bindir}/*"

