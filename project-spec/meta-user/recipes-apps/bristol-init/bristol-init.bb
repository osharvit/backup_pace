#
# This file is the bristol-init recipe.
#

SUMMARY = "Simple bristol-init application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://bristol-init.sh \
	   file://led_blink.sh \ 
		  "
S = "${WORKDIR}"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
inherit update-rc.d
INITSCRIPT_NAME = "bristol-init.sh"
INITSCRIPT_PARAMS = "start 99 S ."
do_install() {
	    install -d ${D}${sysconfdir}/init.d
	    install -m 0755 ${S}/bristol-init.sh ${D}${sysconfdir}/init.d/bristol-init.sh
	  install -d ${D}${bindir}
	  install -m 0755 led_blink.sh ${D}${bindir}
	    
}

INSANE_SKIP_${PN} += "arch"
INSANE_SKIP_${PN} += "file-rdeps"
INSANE_SKIP_${PN}-dev += "arch"
INSANE_SKIP_${PN}-dev += "file-rdeps"

FILES_${PN} += "${sysconfdir}/*"
FILES_${PN} += "${bindir}/*"
