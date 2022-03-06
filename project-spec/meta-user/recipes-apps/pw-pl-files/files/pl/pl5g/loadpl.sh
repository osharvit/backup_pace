#!/bin/sh

# echo "load FPGA + DRV + dtbo"
# #get fpga bin file
# local_f="$(ls  /lib/firmware/base/*.bin)" 
# fpgautil -o /lib/firmware/base/base.dtbo -b $local_f
# exit
# mkdir /mnt/ramdisk
# mount -t tmpfs -o size=20m tmpfs /mnt/ramdisk




echo "load FPGA + DRV + dtbo"                        

firmware_name="pace_top_28_11_2021.bit.bin"
dtbofile="pace_top_28_11_2021.dtbo"
                                         
ln -s pl.bit.bin $firmware_name
ln -s pl.dtbo $dtbofile

fpgautil -R #-R                    Optional: Remove overlay from a live tree
fpgautil -o $dtbofile -b $firmware_name
find -type l -exec rm {} \;


 
