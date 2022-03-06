#
# This file is the banner recipe.
#

SUMMARY = "Simple banner application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://banner.c \
	   file://Makefile \
	  	  "
		  
FILE_VER_GEN ="${THISDIR}/files/version.txt"
		  
S = "${WORKDIR}"


#function gen_version_file {

gen_version_file() {

    FILE_VER="${THISDIR}/files/version_info.c"

    #       git rev-parse --abbrev-ref --symbolic-full-name @{u}
    git_branch_d="$(git rev-parse --abbrev-ref HEAD)"
    git_commit_d="$(git rev-parse --verify HEAD)"
    git_tag_d="$(git --no-pager describe --tags --always)"
    git_commit_date_d="$(git --no-pager show --date=iso8601 --format="%ad" --name-only |head -n 1)"
    build_date_d="$(date --iso=seconds)"


cat <<EOF >$FILE_VER
/* PW version_info file */
/* version_info.c */
#include "version_info.h"

const char * git_branch = "${git_branch_d}";
const char * git_commit = "${git_commit_d}";
const char * git_tag = "${git_tag_d}";
const char * git commit_date = "${git_commit_date_d}";
const char * build_date = "${build_date_d}";
const char * name_recipe = "${PN}";
const char * revision_recipe = "${PR}";
const char * version_recipe = "${PV}";
EOF



    cp ${FILE_VER} $1

}


gen_version_general_file() {


    #       git rev-parse --abbrev-ref --symbolic-full-name @{u}
    git_branch_d="$(git rev-parse --abbrev-ref HEAD)"
    git_commit_d="$(git rev-parse --verify HEAD)"
    git_tag_d="$(git --no-pager describe --tags --always)"
    git_commit_date_d="$(git --no-pager show --date=iso8601 --format="%ad" --name-only |head -n 1)"
    build_date_d="$(date --iso=seconds)"	


cat <<EOF >$1
/* PW version info file */
/* version.txt */

git_branch = "${git_branch_d}";
git_commit = "${git_commit_d}";
git_tag = "${git_tag_d}";
git commit_date = "${git_commit_date_d}";
build_date = "${build_date_d}";
EOF



    cp $1 $2

}


do_compile() {
	 #    gen_version_file ${S}
	 #    gen_version_general_file ${FILE_VER_GEN} ${S}
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -d ${D}/var/log
	     install -m 0755 banner ${D}${bindir}
#	     install -m 0555 version.txt ${D}
}

#FILES_${PN} += "/*"
