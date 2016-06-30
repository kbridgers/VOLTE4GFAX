#!/usr/bin/env bash

#cp ./gphy_firmware.img ../build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/gphy_firmware.img
#cp ./gphy_firmware.img ../build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/gphy_firmware.img
#cp ./gphy_firmware.img ../build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/ltq_fw_PHY_IP-1.4/xRx2xx/gphy_firmware.img
#cp ./gphy_fw_fe.h  ../build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/linux-2.6.32.42/include/switch_api/gphy_fw_fe.h
#cp ./GRX288_UGW5.4_Telit_Driver/libubox-2013-04-15-dcf93f332270bdaddb5d24fdba6e3eb5b1f7d80a.tar.gz ../dl
#cp ./GRX288_UGW5.4_Telit_Driver/uqmi-2013-03-05-b61b3e8ff2b29e08b53eabc7b813c1c87c734947.tar.gz ../dl
cp ./GRX288_UGW5.4_Telit_Driver/qmi_wwan.c ../build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/linux-2.6.32.42/drivers/net/usb/qmi_wwan.c
cp ./GRX288_UGW5.4_Telit_Driver/option.c ../build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/linux-2.6.32.42/drivers/usb/serial/option.c
cd ../
#patch  -p1  < ./ciou_flte_patches/GRX288_UGW5.4_Telit_Driver/uqmi.patch
cd ./build_dir/linux-ltqcpe_vrx288_gw_he_vdsl_lte/linux-2.6.32.42
patch  -p1 < ../../../ciou_flte/GRX288_UGW5.4_Telit_Driver/qmi_wwan_v3.9r2e-for-linux-2.6.32.patch
