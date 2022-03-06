#
# This file is the pw-pl-files recipe.
#

SUMMARY = "Simple pw-pl-files application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"


SRC_URI = "file://pl/pl4g/pl.bit.bin \
	   file://pl/pl4g/pl.dtbo \
	   file://pl/pl4g/loadpl.sh \
		  "
SRC_URI += "file://pl/pl5g/pl.bit.bin"
SRC_URI += "file://pl/pl5g/pl.dtbo"
SRC_URI += "file://pl/pl5g/loadpl.sh"


PL_FINAL = "home/pl_versions"


S = "${WORKDIR}"

do_install() {

	LSL=$(find ${S}/pl/ )
	echo "${LSL}" > ${S}/e.txt
	echo ${LSL}
	declare -a FILE_S
	eval "FILE_S=($LSL)"
	
	echo "${LSL}" > ${S}/ddd.txt
	echo "${#FILE_S[@]} files" > ${S}/bbb.txt
	for each in "${FILE_S[@]}"
	      do
		  echo "$each" > ${S}/aaaa.txt
		  new_file=${each#${S}/pl}
		  echo "$new_file" > ${S}/aaaaaaaaaaaaaa.txt

		  if [ -d "$each" ]; then
		      install -d ${D}/${PL_FINAL}/${new_file}
		  fi
		  if [ -f "$each" ]; then
		      install -m 0655 ${each} ${D}/${PL_FINAL}/${new_file}
		    fi
	      done

}

INSANE_SKIP_${PN} += "arch"
INSANE_SKIP_${PN} += "file-rdeps"
INSANE_SKIP_${PN}-dev += "arch"
INSANE_SKIP_${PN}-dev += "file-rdeps"


FILES_${PN} += "/${PL_FINAL}/*"




