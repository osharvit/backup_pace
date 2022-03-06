#!/bin/bash

GPIO=365
GPIODIR=/sys/class/gpio/gpio$GPIO

echo "Configuring GPIO $GPIO"

#check if the gpio is already exported
if [ ! -e "$GPIODIR" ]
then
	echo "Exporting GPIO"
	echo $GPIO > /sys/class/gpio/export
else
	echo "GPIO already exported"
fi

echo "Current direction: `cat $GPIODIR/direction`"
echo "Set GPIO as output"
echo out > $GPIODIR/direction
echo "New GPIO direction: `cat $GPIODIR/direction`"
echo "Current value: `cat $GPIODIR/value`"
echo "Set value as high"
echo 1 > $GPIODIR/value
echo "New value: `cat $GPIODIR/value`"

#Endless loop
#echo "Start blinking, 1 sec on plus 1 sec off, press CTRL+C to end"
while ( true );
	do echo 1 > $GPIODIR/value;
#	cat $GPIODIR/value;
	sleep 0.5;
	echo 0 > $GPIODIR/value;
#	cat $GPIODIR/value;
	sleep 0.5;
done;
