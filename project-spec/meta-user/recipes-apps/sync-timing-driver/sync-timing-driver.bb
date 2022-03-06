#
# This file is the sync-timing-driver recipe.
#

SUMMARY = "Simple sync-timing-driver application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://sync_timing_driver.tar.gz"

S = "${WORKDIR}"

do_compile() {
	     source ${S}/sync_timing_driver_env_pw.sh
	     make all
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 ${S}/output/sync_timing_driver_app/sync_timing_driver_app ${D}${bindir}
	     install -d ${D}/etc
	     install -m 0555 ${S}/cfg/sync_timing_driver.conf ${D}/etc
	     
}


INSANE_SKIP_${PN} += "arch"
INSANE_SKIP_${PN} += "file-rdeps"
INSANE_SKIP_${PN}-dev += "arch"
INSANE_SKIP_${PN}-dev += "file-rdeps"


FILES_${PN} += "/etc/*"



