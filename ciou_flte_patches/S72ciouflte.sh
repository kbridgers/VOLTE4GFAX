ifconfig br0 down && ifconfig eth0 192.168.1.1
echo "Ethernet is Up Now : Ip 192.168.1.1"
echo "waiting for USB device to enumerate"
echo "ATE0\r\n" > /dev/ttyUSB3
cp /usr/sbin/tapidemopipe /tmp
echo "Satrting Ciou_FLTE applications"
LTEManager &

