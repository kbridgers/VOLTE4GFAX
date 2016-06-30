flash_erase /dev/mtd5 0 0 &> /dev/null
touch /tmp/bootmode.txt
echo "VVDN9" > /tmp/bootmode.txt
sleep 1;
dd if=/tmp/bootmode.txt of=/dev/mtd5 bs=512

