#
# This file is the PACE CLI application recipe.
#

SUMMARY = "PACE CLI application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"


DEPENDS += "lua-native lua"
DEPENDS += "readline"
DEPENDS += "libpaceipc"


#	file://lua/libs/liblua_pace.a

SRC_URI = " \
	file://lua/src/lapi.h			\
	file://lua/src/lauxlib.h		\
	file://lua/src/lcode.h			\
	file://lua/src/lctype.h			\
	file://lua/src/ldebug.h			\
	file://lua/src/ldo.h			\
	file://lua/src/lfunc.h			\
	file://lua/src/lgc.h			\
	file://lua/src/ljumptab.h		\
	file://lua/src/llex.h			\
	file://lua/src/llimits.h		\
	file://lua/src/lmem.h			\
	file://lua/src/lobject.h		\
	file://lua/src/lopcodes.h		\
	file://lua/src/lopnames.h		\
	file://lua/src/lparser.h		\
	file://lua/src/lprefix.h		\
	file://lua/src/lstate.h			\
	file://lua/src/lstring.h		\
	file://lua/src/ltable.h			\
	file://lua/src/ltm.h			\
	file://lua/src/lua.h			\
	file://lua/src/lua.hpp			\
	file://lua/src/luaconf.h		\
	file://lua/src/lualib.h			\
	file://lua/src/lundump.h		\
	file://lua/src/lvm.h			\
	file://lua/src/lzio.h			\
						\
	file://Makefile				\
	file://path.h				\
	file://path.cpp				\
	file://cmdeditor.h			\
	file://cmdeditor.cpp			\
	file://clicpmgr.cpp			\
	file://clicpmgr.h			\
	file://cmd.cpp				\
	file://cmd.h				\
	file://cmdmgr.cpp			\
	file://cmdmgr.h				\
	file://extcmdmgr.cpp			\
	file://extcmdmgr.h			\
	file://file.cpp				\
	file://file.h				\
	file://gen-types.h			\
	file://h2mgr.cpp			\
	file://h2mgr.h				\
	file://helpmgr.cpp			\
	file://helpmgr.h			\
	file://hwmgr.cpp			\
	file://hwmgr.h				\
	file://json.cpp				\
	file://json.h				\
	file://lexer.cpp			\
	file://lexer.h				\
	file://luamgr.cpp			\
	file://luamgr.h				\
	file://luasockets.cpp			\
	file://luasockets.h			\
	file://main.cpp				\
	file://math.cpp				\
	file://math.h				\
	file://rc_cmdmgr.cpp			\
	file://rc_cmdmgr.h			\
	file://retcodes.h			\
	file://socket.cpp			\
	file://socket.h				\
	file://sync.cpp				\
	file://sync.h				\
	file://thread.cpp			\
	file://thread.h				\
	file://varmgr.cpp			\
	file://varmgr.h				\
	file://luaipcmgr.cpp			\
	file://luaipcmgr.h			\
	file://ipcmgr.cpp			\
	file://ipcmgr.h				\
	file://cli_ipc_ctrl_inf.h		\
	"

S = "${WORKDIR}"

FILESEXTRAPATHS_prepend := "${THISDIR}:"
SRC_URI += "file://lua/lua-5.4.3.tar.gz"
#SRC_URI += "git://github.com/lua/lua.git;branch=master "
#SG = "${WORKDIR}/git"


#SRC_URI += "file://extra_files/*"
#SRC_URI += "file://extra_files/hw/*"
#SRC_URI += "file://extra_files/scripts/*"




SRCREV = "eadd8c7178c79c814ecca9652973a9b9dd4cc71b"
SG = "${WORKDIR}/lua-5.4.3"
LUA_VERSION = "5.4.3"
inherit autotools pkgconfig

EXTRA_OEMAKE += "LUA_V=${LUA_VERSION}"
#CFLAGS_append = " -I${RECIPE_SYSROOT}/usr/include -fPIC"
CFLAGS_append = " -I${RECIPE_SYSROOT}/usr/include "

EXTRA_OEMAKE += "'CC=${CC} -fPIC' 'CFLAGS=${CFLAGS} -DLUA_USE_LINUX -fPIC' MYLDFLAGS='${LDFLAGS}'"

PACE_CLI_DIR="/PACE-BIN/cli/pacecli"

do_fetch(){

# generate pacecli
    mkdir -p ${S}/extra_files/
    cp -r ${THISDIR}/files/extra_files ${S}

}

do_compile() {
    cd ${SG}
    oe_runmake clean
#generate only the lib 
    oe_runmake a
    mkdir -p ${S}/lua/libs
    cp ${SG}/*a ${S}/lua/libs/liblua_pace.a 
## generate pacecli
    cd ${S}
    oe_runmake
#    mkdir -p ${S}/extra_files/
#    cp -r ${THISDIR}/files/extra_files ${S}
}

do_install() {

	install -d ${D}${includedir}/pacecli/
	install -m 0644 ${S}/cli_ipc_ctrl_inf.h ${D}${includedir}/pacecli/


#        install -d ${D}${bindir}
	install -d ${D}${PACE_CLI_DIR}
        install -m 0755 ${S}/pacecli ${D}${PACE_CLI_DIR}
#	LSL=$(find ${S}/extra_files/* -print0)
	LSL=$(find ${S}/extra_files/ )
#	echo "${LSL}" > ${S}/e.txt
	echo ${LSL}
	declare -a FILE_S
	eval "FILE_S=($LSL)"
	
	echo "${LSL}" > ${S}/ddd.txt
#	echo "${#FILE_S[@]} files" > ${S}/bbb.txt
	for each in "${FILE_S[@]}"
	      do
#		  echo "$each" > ${S}/aaaa.txt
		  new_file=${each#${S}/extra_files}
#		  echo "$new_file" > ${S}/aaaaaaaaaaaaaa.txt

		  if [ -d "$each" ]; then
		      install -d ${D}${PACE_CLI_DIR}/${new_file}
		  fi
		  if [ -f "$each" ]; then
		      install -m 0655 ${each} ${D}${PACE_CLI_DIR}/${new_file}
		    fi
	      done
}
# + is also for bindir
FILES_${PN} += "${PACE_CLI_DIR}/"

#FILES_${PN} = "${PACE_CLI_DIR}/"
#FILES_${PN} += "${PACE_CLI_DIR}/hw"
#FILES_${PN} += "${PACE_CLI_DIR}/scripts"





