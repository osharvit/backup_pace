FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://platform-top.h \
            file://user_2021-08-23-08-41-00.cfg \
            file://user_2021-12-08-15-37-00.cfg \
            file://user_2021-12-12-16-10-00.cfg \
            file://user_2021-12-13-05-38-00.cfg \
            file://user_2021-12-13-07-10-00.cfg \
            file://user_2021-12-13-12-00-00.cfg \
            file://user_2021-12-13-12-33-00.cfg \
            file://user_2021-12-13-13-07-00.cfg \
            file://user_2022-01-04-10-58-00.cfg \
            "
SRC_URI += "file://bsp.cfg"
SRC_URI += "file://clkB.patch"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

do_configure_append () {
	if [ "${U_BOOT_AUTO_CONFIG}" = "1" ]; then
		install ${WORKDIR}/platform-auto.h ${S}/include/configs/
		install ${WORKDIR}/platform-top.h ${S}/include/configs/
        else
                install ${WORKDIR}/platform-top.h ${S}/include/configs/
	fi
}

do_configure_append_microblaze () {
	if [ "${U_BOOT_AUTO_CONFIG}" = "1" ]; then
		install -d ${B}/source/board/xilinx/microblaze-generic/
		install ${WORKDIR}/config.mk ${B}/source/board/xilinx/microblaze-generic/
	fi
}
