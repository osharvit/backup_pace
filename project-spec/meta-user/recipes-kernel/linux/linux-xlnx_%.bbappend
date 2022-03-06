SRC_URI += "file://user_2021-08-23-06-36-00.cfg \
            file://user_2021-12-19-13-49-00.cfg \
            file://user_2021-12-21-09-08-00.cfg \
            "
SRC_URI += " file://user_2021-12-16-09-02-00.cfg"
SRC_URI += " file://user_2021-12-16-13-41-00.cfg"

SRC_URI += "file://kernel.cfg"

# This line adds a kernel patch
SRC_URI_append = " file://xilinx_ddma.patch"
#SRC_URI_append = " file://0001-Signed-off-by-Xu-Dong-xud-xilinx.com.patch"
#SRC_URI_append = " file://0001-Re-apply-fix-to-changed-baseline-code.patch"
#SRC_URI_append = " file://0001-net-xilinx-axiethernet-Workaround-for-NOOP-timestamp.patch"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

