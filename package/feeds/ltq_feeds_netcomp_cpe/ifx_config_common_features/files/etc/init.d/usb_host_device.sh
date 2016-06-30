#!/bin/sh /etc/rc.common
START=70
OUT="/tmp/cmd_output"
flush_output()
{
    echo "" > "$OUT"
}
remove_output()
{
    rm -f "$OUT"
}
SED="/bin/sed"

SATA_CLASS="0106"
USB30_CLASS="0c03"

start() {

	if [ $usb0Mode -eq 1 -o $usb1Mode -eq 1 ]; then
		if [ -f /lib/modules/*/ifxusb_host.ko ]; then
			[ $usb0Mode -eq 1 ] && echo -n "USB0: " || echo -n "USB1: "; echo "Loading USB Host mode driver"
			insmod /lib/modules/*/ifxusb_host.ko
		fi
	fi

	lspci 2>/dev/null | grep -qw "$SATA_CLASS:" && {
		echo "Found SATA module connected on PCI(e)"
		if [ -f /lib/modules/*/ahci.ko ]; then
			insmod /lib/modules/*/ahci.ko
		fi
	} || true

	lspci 2>/dev/null | grep -qw "$USB30_CLASS:" && {
		echo "Found USB module connected on PCI(e)"
		if [ -f /lib/modules/*/xhci.ko ]; then
			insmod /lib/modules/*/xhci.ko
		fi
	} || true

flush_output
ifconfig eth0 > "$OUT"
MAC_ADDR_BASE=`"$SED" -n 's,^.*HWaddr,,;1p' "$OUT"`
USB_MAC_ADDR=`echo $MAC_ADDR_BASE|/usr/sbin/next_macaddr -7`

	if [ $usb0Mode -eq 2 -o $usb1Mode -eq 2 ]; then
		if [ -f /lib/modules/*/ifxusb_gadget.ko ] && [ -f /lib/modules/*/g_ether.ko ]; then
			[ $usb0Mode -eq 2 ] && echo -n "USB0: " || echo -n "USB1: "; echo "Loading USB Device mode driver"
			/sbin/insmod /lib/modules/*/ifxusb_gadget.ko
			board_mac=`/usr/sbin/upgrade mac_get 0`
			/sbin/insmod /lib/modules/*/g_ether.ko dev_addr="$USB_MAC_ADDR"
			/usr/sbin/brctl addif $lan_main_0_interface usb0
			/sbin/ifconfig usb0 0.0.0.0 up
		fi
	fi
remove_output
}
