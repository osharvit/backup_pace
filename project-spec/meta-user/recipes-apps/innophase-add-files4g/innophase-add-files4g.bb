#
# This file is the innophase-add-files recipe.
#
SUMMARY = "innophase-add-files application"
DESCRIPTION = "add on files"
SECTION = "PETALINUX/apps"
LICENSE = "CLOSED"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS_prepend := "${THISDIR}/files/innophase_files:"
SRC_URI = "file://innophase_files/_bashrc "
SRC_URI += "file://innophase_files/hermes2_startup.sh "
SRC_URI += "file://innophase_files/_profile "


#SRC_URI += "file://*"
inherit allarch  
S = "${WORKDIR}"
INNO = "innophase4g"
#INNO_FINAL = "home/root"
INNO_FINAL = "innophase/innophase4g"



do_install() {
#create folder
	# install -d ${D}/${INNO}
	      

	cp -R ${THISDIR}/files/innophase_files/ ${S}/
	LSL=$(find ${S}/innophase_files/ )
	echo "${LSL}" > ${S}/e.txt
	echo ${LSL}
	declare -a FILE_S
	eval "FILE_S=($LSL)"
	
	echo "${LSL}" > ${S}/ddd.txt
	echo "${#FILE_S[@]} files" > ${S}/bbb.txt
	for each in "${FILE_S[@]}"
	      do
		  echo "$each" > ${S}/aaaa.txt
		  new_file=${each#${S}/innophase_files}
		  echo "$new_file" > ${S}/aaaaaaaaaaaaaa.txt

		  if [ -d "$each" ]; then
		      install -d ${D}/${INNO_FINAL}/${new_file}
		  fi
		  if [ -f "$each" ]; then
		      install -m 0655 ${each} ${D}/${INNO_FINAL}/${new_file}
		    fi
	      done

}

INSANE_SKIP_${PN} += "arch"
INSANE_SKIP_${PN} += "file-rdeps"
INSANE_SKIP_${PN}-dev += "arch"
INSANE_SKIP_${PN}-dev += "file-rdeps"


FILES_${PN} += "/${INNO_FINAL}/*"










