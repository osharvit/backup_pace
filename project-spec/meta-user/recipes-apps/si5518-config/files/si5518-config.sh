#!/bin/sh
echo "Initialization of Si5518" > /dev/ttyPS0
sleep 1
sync_timing_util -d /si5518-config/Si5518B-HW_Bring_Up_Config-PWv002-prod_fw.boot.bin /si5518-config/Si5518B-HW_Bring_Up_Config-PWv002-user_config.boot.bin
sync_timing_util -v > /dev/ttyPS0

