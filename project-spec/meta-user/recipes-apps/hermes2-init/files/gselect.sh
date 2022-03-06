#!/bin/sh

INNO5GFOLDER=/innophase/innophase5g
INNO4GFOLDER=/innophase/innophase4g
PL4G=/home/pl_versions/pl4g
PL5G=/home/pl_versions/pl5g
H2APP4G=/usr/bin/h2app4g.elf
H2APP5G=/usr/bin/h2app5g.elf


cd /home/root
#delete all symboliclink 
find -type l -exec rm {} \;
if [ -e /usr/bin/h2app.elf ]; then
  rm /usr/bin/h2app.elf
fi
read -p "please choose one 5G or 4G - enter 4 or 5 : " gname
echo "choosen ${gname}G!"

if [ ${gname} -eq "5" ] ; then
  
  INNO=$INNO5GFOLDER
  PL=$PL5G
  H2=$H2APP5G
else
  INNO=$INNO4GFOLDER
  PL=$PL4G
  H2=$H2APP4G


fi

ln -s $H2 /usr/bin/h2app.elf
ln -s $PL /home/root/pl_version
LSL=$(ls $INNO/ )
#echo "${LSL}" > /e.txt
#echo ${LSL}
declare -a FILE_S
eval "FILE_S=($LSL)"

#echo "${LSL}" > /ddd.txt
#echo "${#FILE_S[@]} files" > /bbb.txt
for each in "${FILE_S[@]}"
      do
#	  echo "$each" >> /aaaa.txt
	  new_file=${each#}
#	  echo "$new_file" >> /aaaaaaaaaaaaaa.txt
	  ln -s ${INNO}/${each} /home/root/${new_file}
      done
source /home/root/hermes2_startup.sh

