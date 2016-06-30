#!/bin/bash
echo "Going to delete unwanted files from rootfs..."
echo "If you have made a secondary filesystem using ./make_sec_rootfs then you may nees to do 'make clean', 'make -j4' and ./make_rootfs.sh  "
./multi_delete.sh ./list_primary.txt
echo "Calling openwrt's rootfs make..."
cp ./ciou_flte_factory_default/vr9_bbd_fxs.bin ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/lib/firmware
cp ./ciou_flte_factory_default/primary_web/* ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/www/
cp ./ciou_flte_factory_default/initscripts/* ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/etc/init.d
cp ./ciou_flte_factory_default/startscripts/* ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/etc/rc.d
cp ./ciouflteversion.txt ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/etc/rc.d
cp ./ciouflteversion.txt ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/etc/banner
rm ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/etc/rc.d/S72node_codec.sh
rm ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/etc/rc.d/S74resetcodecconfig.sh

make rootfs V=99
rm -rf ./root.squashfs
if [ -f ./build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/root.squashfs ];then
cp -f build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/root.squashfs ./ 
else
echo "Cannot find rootfs image \"root.squashfs\".It mightnot not be build by OpenWrt!"
fi

