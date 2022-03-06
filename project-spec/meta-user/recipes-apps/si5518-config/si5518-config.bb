#
# This file is the si5518-config recipe.
#

SUMMARY = "Simple si5518-config application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://si5518-config.sh \
	   file://si5518-config \
	   file://si5518-config/Si5518B-HW_Bring_Up_Config-PWv002-prod_fw.boot.bin \
	   file://si5518-config/Si5518B-HW_Bring_Up_Config-PWv002-user_config.boot.bin \
		  "

S = "${WORKDIR}"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"


do_install() {
 install -d ${D}${bindir}
 install -m 0755 si5518-config.sh ${D}${bindir}
 install -d ${D}/si5518-config
 install -m 0666 ${WORKDIR}/si5518-config/Si5518B-HW_Bring_Up_Config-PWv002-prod_fw.boot.bin ${D}/si5518-config/Si5518B-HW_Bring_Up_Config-PWv002-prod_fw.boot.bin
 install -m 0666 ${WORKDIR}/si5518-config/Si5518B-HW_Bring_Up_Config-PWv002-user_config.boot.bin ${D}/si5518-config/Si5518B-HW_Bring_Up_Config-PWv002-user_config.boot.bin
}
FILES_${PN} += "/si5518-config"

