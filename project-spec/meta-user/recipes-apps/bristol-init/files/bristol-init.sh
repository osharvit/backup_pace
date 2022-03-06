#!/bin/sh
echo "Initialization of Bristol board" > /dev/ttyPS0
#si5518 clock script
si5518-config.sh
#5391 clock A
clk5391.sh
#IO Expanders
tca.sh
#Read  temperature
tempr.sh
#led blink finish linux
led_blink.sh &

