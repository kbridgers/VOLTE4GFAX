#!/bin/sh

VINAX_VDSL_MODE=$1
VINAX_ADSL_MODE=$2
VINAX_DEBUG_LEVEL=$3

echo "- VDSL -"

# drv_rb_xrx100
echo "- loading drv_rb_xrx100.ko -"
/sbin/insmod /lib/modules/vdsl/drv_rb_xrx100.ko
if [ $? -ne 0 ]; then
	echo "- loading drv_rb_xrx100.ko failed! -"
	exit 1
fi

# lib_ifxos
echo "- loading drv_ifxos.ko -"
/sbin/insmod /lib/modules/vdsl/drv_ifxos.ko
if [ $? -ne 0 ]; then
	echo "- loading drv_ifxos.ko failed! -"
	exit 1
fi

# drv_vinax
echo "- loading drv_vinax.ko - "
rm -rf /dev/vinax
mkdir -p /dev/vinax
/sbin/insmod /lib/modules/vdsl/drv_vinax.ko debug_level=$VINAX_DEBUG_LEVEL
if [ $? -ne 0 ]; then
	echo "- loading drv_vinax.ko failed! -"
	exit 1
fi
if [ ! -e /dev/vinax/0 ]; then
	mknod_util vinax /dev/vinax/0 0
	mknod_util vinax /dev/vinax/cntrl0 128
#	mknod_util vinax /dev/vinax/1 1
#	mknod_util vinax /dev/vinax/cntrl0 129
fi
sleep 1

# initialize VINAX driver
#   VINAX base address: 0x1c000000
#   IRQ Line: 0x63 (polling) 0x9e (EXIN0) 0x21 (EXIN4)
vinax_drv_test -i 0x1c000000 -o 0x21 -n 0
#   Patch for PLLB UnLock issue - IFX-joelin. Removed seems not required.
#   vinax_drv_test -m 0xe2000 -D 2 -x 1 -o 0x402
sleep 1

# drv_dsl_cpe_api
echo "- loading drv_dsl_cpe_api.ko "
rm -rf /dev/dsl_cpe_api
mkdir -p /dev/dsl_cpe_api
/sbin/insmod /lib/modules/vdsl/drv_dsl_cpe_api.ko
if [ $? -ne 0 ]; then
	echo "- loading drv_dsl_cpe_api.ko failed! -"
	exit 1
fi
if [ ! -e /dev/dsl_cpe_api/0 ]; then
	mknod_util drv_dsl_cpe_api /dev/dsl_cpe_api/0 0
#	mknod_util drv_dsl_cpe_api /dev/dsl_cpe_api/1 1
fi

XDSL_XTSE=
XDSL_FW=
XDSL_SCR=
case $VINAX_VDSL_MODE in
1)
	VDSL_XTSE=07
	VDSL_FW="-f /vdsl/vcpe_hw_v9.10.3.12.0.2.bin"
	VDSL_SCR="-A /vdsl/vdsl.scr"
	;;
*)
	VDSL_XTSE=00
	VDSL_FW=
	VDSL_SCR=
	;;
esac
case $VINAX_ADSL_MODE in
1)
	ADSL_XTSE=05_01_04_01_0C_01_00
	ADSL_FW="-F /vdsl/acpe_hw_v9.6.1.2.0.5.bin"
	ADSL_SCR="-a /vdsl/adsl.scr"
	;;
2)
	ADSL_XTSE=10_00_10_00_00_04_00
	ADSL_FW="-F /vdsl/acpe_hw_v9.5.3.0.0.6.bin"
	ADSL_SCR="-a /vdsl/adsl.scr"
	;;
*)
	ADSL_XTSE=00_00_00_00_00_00_00
	ADSL_FW=
	ADSL_SCR=
esac

XDSL_XTSE="-i${ADSL_XTSE}_${VDSL_XTSE}"
XDSL_FW="${VDSL_FW} ${ADSL_FW}"
XDSL_SCR="${VDSL_SCR} ${ADSL_SCR}"

echo "- running vdsl_cpe_control -"
/usr/sbin/vdsl_cpe_control -n"/vdsl/vdslrc.sh" ${XDSL_XTSE} ${XDSL_FW} ${XDSL_SCR} &

