echo "Starting Upgrade!!!!!! Please Wait Some Time........"
flash_erase /dev/mtd3 0 0 &> /dev/null
flash_erase /dev/mtd4 0 0 &> /dev/null 
dd if=/tmp/fw of=/dev/mtd4 bs=512 count=12288 skip=4096
dd if=/tmp/fw of=/dev/mtd3 bs=512 count=4096 
flash_erase /dev/mtd5 0 0 &> /dev/null
touch /tmp/bootmode.txt
echo "VVDN0" > /tmp/bootmode.txt
sleep 1;
dd if=/tmp/bootmode.txt of=/dev/mtd5 bs=512
echo "upgrade_Successfull"
