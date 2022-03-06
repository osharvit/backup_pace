#include <configs/xilinx_zynqmp.h>
//#include <configs/platform-auto.h>
#ifdef BOOTENV_DEV_QSPI
#undef BOOTENV_DEV_QSPI
#endif
#define BOOTENV_DEV_QSPI(devtypeu, devtypel, instance) \
        "bootcmd_" #devtypel #instance "=sf probe 2 0 0 && " \
                       "sf read $scriptaddr $script_offset_f $script_size_f && " \
                       "source ${scriptaddr}; echo SCRIPT FAILED: continuing...;\0"

