#!/bin/bash
make -j4 V=99
cp ./ciou_flte_factory_default/ifx_cgi_system.c ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/ifx_httpd-2.3.0.85/ 
make package/feeds/ltq_feeds_netcomp_cpe/ifx_httpd/{compile,install} V=99
echo "Going to delete unwanted files from rootfs for secondary root Filesystem..."
./multi_delete.sh ./list_secondary.txt
echo "Calling openwrt's rootfs make..."
cp ./ciou_flte_factory_default/vr9_bbd_fxs.bin ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/lib/firmware
cp ./ciou_flte_factory_default/secondary_web/* ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/www/
cp ./ciou_flte_factory_default/startscripts_sec/* ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/etc/rc.d
cp ./ciouflteversion.txt ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/etc/rc.d
cp ./ciouflteversion.txt ./build_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/root-ltqcpe/etc/banner
make rootfs V=99
rm -rf ./root.squashfs
if [ -f ./build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/root.squashfs ];then
cp -f build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/root.squashfs ./ 
else
echo "Cannot find rootfs image \"root.squashfs\".It mightnot not be build by OpenWrt!"
fi

