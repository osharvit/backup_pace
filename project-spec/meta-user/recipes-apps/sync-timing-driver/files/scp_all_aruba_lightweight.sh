#!/bin/bash
#
# Purpose: To scp application binaries and other files to the DUT
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

DUT_ADDR=$1
DUT_PASSWORD="root"
DUT_LOCATION="/usr/bin/"
echo "Host IP Address :" $DUT_ADDR
START_CORE=1
if [ -z "$2" ]
then
    START_CORE=1
else
    START_CORE=$2
fi

if [ -z "$3" ]
then
    DUT_LOCATION="/usr/bin"
else
    DUT_LOCATION="/mnt/sdcard/files/"
fi

echo "START_CORE = " $START_CORE
echo "DUT_LOCATION = " $DUT_LOCATION

echo "Terminate any old regular driver if running"
sshpass -p $DUT_PASSWORD ssh root@$DUT_ADDR "/etc/init.d/synctimingdriver.sh stop"

echo "Copying sync_timing_driver_app to DUT"
sshpass -p $DUT_PASSWORD scp output/sync_timing_driver_app/sync_timing_driver_app root@$DUT_ADDR:$DUT_LOCATION

echo "Copying test script to DUT"
sshpass -p $DUT_PASSWORD scp apps/sync_timing_driver_app/test/example_test_script.txt root@$DUT_ADDR:$DUT_LOCATION

echo "Copying Driver Conf files to DUT"
if [ -f cfg/sync_timing_driver_aruba_lightweight.conf ]
then
	sshpass -p $DUT_PASSWORD scp cfg/sync_timing_driver_aruba_lightweight.conf  root@$DUT_ADDR:/etc/sync_timing_driver.conf
else
	if [ -f cfg/sync_timing_driver.conf ] # release package
	then
		sshpass -p $DUT_PASSWORD scp cfg/sync_timing_driver.conf  root@$DUT_ADDR:/etc/sync_timing_driver.conf
	fi
fi

echo "Mounting /dev/mqueue/"
sshpass -p $DUT_PASSWORD ssh root@$DUT_ADDR "mount -t mqueue none /dev/mqueue"

echo "All Done"



