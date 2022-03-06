i2cset -y -f 0 0x70 0x10
t1=`i2cget -y -f 0 0x48 0`
printf "\nTemperature I2C bus 0 port 4 %d C\n" $t1 >/dev/ttyPS0
printf "\nTemperature I2C bus 0 port 4 %d C\n" $t1
i2cset -y -f 1 0x70 0x08
t2=`i2cget -y -f 0 0x48 0`
printf "Temperature I2C bus 1 port 3 %d C\n" $t2 >/dev/ttyPS0
printf "Temperature I2C bus 1 port 3 %d C\n" $t2
i2cset -y -f 1 0x70 0x80
t3=`i2cget -y -f 0 0x48 0`
printf "Temperature I2C bus 1 port 7 %d C\n" $t3 >/dev/ttyPS0
printf "Temperature I2C bus 1 port 7 %d C\n" $t3

