#
# This file is the clk5391 recipe.
#

SUMMARY = "clk5391 configuration script"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://clk5391.sh \
 "
S = "${WORKDIR}"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
#inherit update-rc.d
#INITSCRIPT_NAME = "clk5391.sh"
#INITSCRIPT_PARAMS = "start 99 S ."
#do_install() {
# install -d ${D}${sysconfdir}/init.d
# install -m 0755 ${S}/clk5391.sh ${D}${sysconfdir}/init.d/clk5391.sh
#}
do_install() {
         install -d ${D}${bindir}
         install -m 0755 clk5391.sh ${D}${bindir}
}
FILES_${PN} += "${sysconfdir}/*"

