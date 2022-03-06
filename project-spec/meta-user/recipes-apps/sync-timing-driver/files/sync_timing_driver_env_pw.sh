#!/bin/bash
#
# Purpose: To configure build environment
#
#/****************************************************************************************/
#/**                  Copyright (c) 2018, 2021 Skyworks Solution Inc.                   **/
#/****************************************************************************************/
#/** This software is provided 'as-is', without any express or implied warranty.        **/
#/** In no event will the authors be held liable for any damages arising from the use   **/
#/** of this software.                                                                  **/
#/** Permission is granted to anyone to use this software for any purpose, including    **/
#/** commercial applications, and to alter it and redistribute it freely, subject to    **/
#/** the following restrictions:                                                        **/
#/** 1. The origin of this software must not be misrepresented; you must not claim that **/
#/**    you wrote the original software. If you use this software in a product,         **/
#/**    an acknowledgment in the product documentation would be appreciated but is not  **/
#/**    required.                                                                       **/
#/** 2. Altered source versions must be plainly marked as such, and must not be         **/
#/**    misrepresented as being the original software.                                  **/
#/** 3. This notice may not be removed or altered from any source distribution.         **/
#/****************************************************************************************/

PKG_FILE="pkg_info_default.mk"

value=`grep SYNC_TIMING_HOST_PLATFORM ./$PKG_FILE | cut -d= -f2`
echo $value
export BUILD_PLATFORM=$value

echo "BUILD_PLATFORM = " $BUILD_PLATFORM
echo "PKG_FILE = " $PKG_FILE

value=`grep SYNC_TIMING_CHIPSET ./$PKG_FILE | cut -d= -f2`
echo $value
export BUILD_CHIPSET=$value

PARSE_FOR_CHIPSET=0
PETALINUX_VERSION=2020.2
ALT_PETALINUX_VERSION=2020.1

if [ "${BUILD_PLATFORM}" == "xilinx_zcu1xx" ] && [ "${BUILD_CHIPSET}" == "si5388" ]
then
    PETALINUX_VERSION=2019.1
    ALT_PETALINUX_VERSION=2018.2
fi


echo "Source Xilinx Petalinux ${PETALINUX_VERSION} environment script"
export SYNC_TIMING_BUILD_PLATFORM=$BUILD_PLATFORM
export SYNC_TIMING_PKG_FILE=$PKG_FILE
PARSE_FOR_CHIPSET=1

value=`grep SYNC_TIMING_CHIPSET ./$PKG_FILE | cut -d= -f2`
echo $value
export SYNC_TIMING_BUILD_CHIPSET=$value

value=`grep SYNC_TIMING_SERVO_PKG_TYPE ./$PKG_FILE | cut -d= -f2`
echo $value
export SYNC_TIMING_BUILD_SERVO_PKG_TYPE=$value


value=`grep SYNC_TIMING_ACCUTIME_SUPPORT ./$PKG_FILE | cut -d= -f2`
echo $value
export SYNC_TIMING_BUILD_ACCUTIME_SUPPORT=$value


