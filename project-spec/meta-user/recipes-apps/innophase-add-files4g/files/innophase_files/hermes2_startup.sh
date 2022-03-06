#!/bin/sh

echo "hello Hermes World"
 
# local variables
STORAGEDIR="/innophase/"
WORKINGDIR="/home/root/"
FPGA_IP="192.168.2.40"
USER_IP="192.168.2.30"

# Factory IP address setup as an alias "eth0:f"
#ifconfig eth0:f ${FPGA_IP} netmask 255.255.255.0
route add default gw ${USER_IP} eth0

# enable "telnetd" using "inetd"
if [ -f /etc/inetd.conf ]; then
    sed -i "s/^#telnet/telnet/g" /etc/inetd.conf
    /etc/init.d/inetd.busybox restart
fi

# add an IP for userpc
if [ "$(grep userpc /etc/hosts)" == "" ]; then
    echo "$USER_IP    userpc" >> /etc/hosts
fi

# Make sure we're in the base working directory
cd ${WORKINGDIR}

# # copy .profile for root
# if [ -f ${STORAGEDIR}_profile ]; then
#     cp ${STORAGEDIR}_profile ${WORKINGDIR}.profile
# fi
# 
# # copy .bashrc for root
# if [ -f ${STORAGEDIR}_bashrc ]; then
#     cp ${STORAGEDIR}_bashrc ${WORKINGDIR}.bashrc
# fi

#exit 1
# Unpack active (or passive) packages (if they exist), 
# otherwise use individual components found in files sub-directories
# if [ -f ${STORAGEDIR}active_sw_pkg.tar.gz.x ]; then
#     echo "Unpacking active_sw_pkg.tar.gz.x"
#     tar zxf ${STORAGEDIR}active_sw_pkg.tar.gz.x -C ${WORKINGDIR}
# elif [ -f ${STORAGEDIR}passive_sw_pkg.tar.gz.x ]; then
#     echo "Unpacking passive_sw_pkg.tar.gz"
#     tar zxf ${STORAGEDIR}passive_sw_pkg.tar.gz.x -C ${WORKINGDIR}
# else
#     echo "Using individual components (not packages) from /files subdirectories"
# 
#     if [ -f ${STORAGEDIR}files/bitstream/pl.bit ]; then
# 	[ ! -d ${WORKINGDIR}bitstream ] && mkdir ${WORKINGDIR}bitstream
# 	cp ${STORAGEDIR}files/bitstream/pl.bit ${WORKINGDIR}bitstream/pl.bit
#     fi
# 
#     if [ -d ${STORAGEDIR}boot ]; then
# 	cp -r ${STORAGEDIR}files/boot ${WORKINGDIR}
# 	chmod +x ${WORKINGDIR}boot/*.elf
#     fi
# 
#     if [ -d ${STORAGEDIR}openocd ]; then
# 	cp -r ${STORAGEDIR}files/openocd ${WORKINGDIR}
# 	chmod +x ${WORKINGDIR}openocd/openocd
#     fi
# 
#     if [ -f ${STORAGEDIR}h2fw/h2fw.elf ]; then
# 	[ ! -d ${WORKINGDIR}h2fw ] && mkdir ${WORKINGDIR}h2fw
# 	cp ${STORAGEDIR}files/h2fw/h2fw.elf ${WORKINGDIR}h2fw/
# 	chmod +x ${WORKINGDIR}h2fw/*.elf
#     fi
# 
#     if [ -f ${STORAGEDIR}h2app/h2app.elf ]; then
# 	[ ! -d ${WORKINGDIR}h2app ] && mkdir ${WORKINGDIR}h2app
# 	cp ${STORAGEDIR}files/h2app/h2app.elf ${WORKINGDIR}h2app/
# 	chmod +x ${WORKINGDIR}h2app/*.elf
#     fi
# 
#     if [ -f ${STORAGEDIR}h2app/nvdata.json ]; then
# 	[ ! -d ${WORKINGDIR}h2app ] && mkdir ${WORKINGDIR}h2app
# 	cp ${STORAGEDIR}files/h2app/nvdata.json ${WORKINGDIR}h2app/
#     fi
# fi

# copy nvdata
#if [ -d ${STORAGEDIR}nvdata ]; then
#    [ ! -d ${WORKINGDIR}nvdata ] && mkdir ${WORKINGDIR}nvdata
#    cp ${STORAGEDIR}nvdata/h2nvdata.json ${WORKINGDIR}nvdata/
#fi

#FILE="h2nvdata.json"
#if [ -f "/$FILE" ]; then
#    echo "$FILE exists"
#    [ ! -d ${WORKINGDIR}nvdata ] && mkdir ${WORKINGDIR}nvdata
#    cp "/$FILE" ${WORKINGDIR}nvdata/
#
#fi


# # copy TM (testmodel) waveforms folder
# if [ -d ${STORAGEDIR}waveforms ]; then
#     cp -r ${STORAGEDIR}waveforms ${WORKINGDIR}
# fi
# 
# # copy bin folder
# if [ -d ${WORKINGDIR}bin ]; then
#     chmod +x ${STORAGEDIR}bin/*
#   #  cp -r ${STORAGEDIR}bin ${WORKINGDIR}
# fi

# FPGA bit file
if [ -f ${WORKINGDIR}bitstream/pl.bit ]; then
    echo "Programming FPGA with bitstream pl.bit"
    #fpgautil -b ${WORKINGDIR}bitstream/pl.bit
   # fpgautil -b ${WORKINGDIR}pl_version/pl.bit.bin -o ${WORKINGDIR}pl_version/pl.dtbo
    cd pl_version
    source ./loadpl.sh
    cd ..
  #  /loadfpga
fi

# openocd 
if [ -f ${WORKINGDIR}openocd/openocd ]; then
    echo "read FPGA PL version"
    FPGAVER=$(peek 0x80400000)
    echo "Launching OpenOCD"
    if [ ${FPGAVER:0:6} == "0x0002" ]; then
        (cd ${WORKINGDIR}openocd; chmod +x openocd; ./openocd -c 'bindto 0.0.0.0' -f innoh2.cfg -f h2-swd.cfg > openocd.log 2>&1 &)
    else
        (cd ${WORKINGDIR}openocd; chmod +x openocd; ./openocd -c 'bindto 0.0.0.0' -f innoh2.cfg -f h2-jtag.cfg > openocd.log 2>&1 &)
    fi
fi

# boot scripts
if [ -d ${WORKINGDIR}boot/script ]; then
    chmod +x ${WORKINGDIR}boot/script/*.py
fi

echo "need to create ln - h2nvdata_bxx.json h2nvdata.json" 

local_f="$(ls  /*.json)" 
if [ -z "$local_f" ]; then
    echo "missing CALIB file h2nvdata_bxx.json"
  else
    echo "remove h2nvdata_bxx.json"
    rm /home/root/nvdata/*json
    echo "cp CALIB file $local_f "
    ln -s $local_f /home/root/nvdata/h2nvdata.json
fi

#poke 0x80800034 0x00050000
# run the fpga_app
#if [ -f ${WORKINGDIR}h2app/h2app.elf ]; then
#    ${WORKINGDIR}h2app/h2app.elf &
#fi

