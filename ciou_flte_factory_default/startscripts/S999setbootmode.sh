flash_erase /dev/mtd5 0 0 &> /dev/null
touch /tmp/bootmodeset.txt
echo "VVDN0" > /tmp/bootmodeset.txt
sleep 1;
dd if=/tmp/bootmodeset.txt of=/dev/mtd5 bs=512
