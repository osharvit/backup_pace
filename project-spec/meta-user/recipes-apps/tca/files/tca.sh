function config_tca6424A() {
#configuration
i2cset -y -f $1 0x22 0x0C 0
i2cset -y -f $1 0x22 0x0D 0xFF
i2cset -y -f $1 0x22 0x0E 0x7D

#set outputs p00 - p07 to 0
i2cset -y -f $1 0x22 0x04 0
#set outputs p21 , p27 to 0
i2cset -y -f $1 0x22 0x06 0
i2cset -y -f $1 0x22 0x01 1
#set PWR_EN_VCCINT to 1

i2cset -y -f $1 0x22 0x04 1
sleep 0.02
#read PGOOD_VCCINT
pgvccint=`i2cget -y -f $1 0x22 0x01`
#dump result read
echo "READ PGOOD_VCCINT " $pgvccint
((pgvccint&=0x01))
echo $pgvccint
if [ $pgvccint  != 1 ]; then
        echo "PWR_EN_VCCINT failed " $pgvccint >/dev/ttyPS0
        return -1
fi

#PWR_EN_VCCAUX
# set constant
i2cset -y -f $1 0x22 0x04 3
sleep 0.02
pgvccaux=`i2cget -y -f $1 0x22 0x01`
echo "READ PWR_EN_VCCAUX " $pgvccaux
((pgvccaux&=0x03))
echo $pgvccaux
if [ $pgvccaux  != 3 ]; then
        echo "PWR_EN_VCCAUX failed " $pgvccaux >/dev/ttyPS0
        return -1
fi


#PWR_EN_MGTAVCC
# set constant
i2cset -y -f $1 0x22 0x04 7
sleep 0.02
#read PGOOD PWR_EN_MGTAVCC
pgmgtavcc=`i2cget -y -f $1 0x22 0x01`
#dump result read
echo "READ PWR_EN_MGTAVCC " $pgmgtavcc
((pgmgtavcc&=0x07))
echo $pgmgtavcc
if [ $pgmgtavcc  != 7 ]; then
        echo "PWR_EN_MGTAVCC failed " $pgmgtavcc >/dev/ttyPS0
        return -1
fi


#PWR_EN_MGTAVTT
i2cset -y -f $1 0x22 0x04 0x0F
sleep 0.02
#read PGOOD PWR_EN_MGTAVTT
pgmgtavtt=`i2cget -y -f $1 0x22 0x01`
#dump result read
echo "PGOOD PWR_EN_MGTAVTT " $pgmgtavtt
((pgmgtavtt&=0x0F))
echo $pgmgtavtt
if [ $pgmgtavtt  != 15 ]; then
        echo "PWR_EN_MGTAVTT failed " $pgmgtavtt >/dev/ttyPS0
        return -1
fi
#PWR_EN_MGTVCCAUX
i2cset -y -f $1 0x22 0x04 0x1F
sleep 0.02
#read PGOOD PWR_EN_MGTVCCAUX
pgmgtvccaux=`i2cget -y -f $1 0x22 0x01`
#dump result read
echo "PGOOD PWR_EN_MGTVCCAUX " $pgmgtvccaux
((pgmgtvccaux&=0x1F))
echo $pgmgtvccaux
if [ $pgmgtvccaux  != 31 ]; then
        echo "PWR_EN_MGTVCCAUX failed " $pgmgtvccaux >/dev/ttyPS0
        return -1
fi

#PWR_EN_VCCO_3V3

i2cset -y -f $1 0x22 0x04 0x3F
sleep 0.02
#PWR_EN_VCCO_2V5
i2cset -y -f $1 0x22 0x06 0x02
sleep 0.02
#PWR_EN_VCCO_1V8
i2cset -y -f $1 0x22 0x04 0x7F
sleep 0.02
#PWR_EN_VCCO_1V2
i2cset -y -f $1 0x22 0x04 0xFF
sleep 0.02
#read PGOOD PWR_EN_VCCO_3V3, 1V8, 1V2
pg3v32v51v81v2=`i2cget -y -f $1 0x22 0x01`
#dump result read
echo "PGOOD PWR_EN_VCCO_3V3 PWR_EN_VCCO_1V8 PWR_EN_VCCO_1V2 " $pg3v32v51v81v2
if [ $pg3v32v51v81v2  != 0xff ]; then
        echo " PWR_EN_VCCO_3V3 PWR_EN_VCCO_1V8 PWR_EN_VCCO_1V2 failed " $pg3v32v51v81v2 >/dev/ttyPS0
        return -1
fi

#read PGOOD PWR_EN_VCCO_2V5
pgvcco2v5=`i2cget -y -f $1 0x22 0x02`
#dump result read
echo "PGOOD PWR_EN_VCCO_2V5 " $pgvcco2v5
((pgvcco2v5&=0x03))
if [ $pgvcco2v5  != 3 ]; then
        echo "PWR_EN_VCCO_2V5 failed " $pgvcco2v5 >/dev/ttyPS0
        return -1
fi
i2cset -y -f $1 0x22 0x06  0x82
return 0


}

echo "Initialization of TCA, Using bus number 1 " >/dev/ttyPS0
#set TCA port #1 on bus#1
echo "Set Mux output to port 1 (0x02)"
i2cset -y -f 1 0x70 0x02
#+++++++++++ set TCA port #2 +++++++++++++++++
config_tca6424A 1
return_val=$?
echo "return_val" $return_val
if [ "$return_val" -eq 255 ]; then
	echo "Configuration of tca address 0x02 failed; exiting" >/dev/ttyPS0
	exit
fi
#+++++++++++ set TCA port #5 +++++++++++++++++
echo "Set Mux output to port 5 (0x20)"
i2cset -y -f 1 0x70 0x20
config_tca6424A 1
return_val=$?

if [ "$return_val" -eq 255 ]; then
	echo "Configuration of tca address 0x20 failed; exiting" >/dev/ttyPS0
	exit
fi
echo "SUccessfully configured TCA port 1 and 5" >/dev/ttyPS0
