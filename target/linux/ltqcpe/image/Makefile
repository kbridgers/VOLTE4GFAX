# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

UGW_VERSION:=UGW-$(shell cat $(TOPDIR)/ugw_version)
JFFS2_BLOCKSIZE:=$(call qstrip,$(CONFIG_TARGET_ROOTFS_IMAGE_PHYSICAL_ERASE_SIZE))
include $(INCLUDE_DIR)/image.mk

ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON_S")
  CPE_NAME:=XRX100
else
  CPE_NAME:=$(CONFIG_IFX_CONFIG_CPU)
endif
UGW_BANNER:=Lantiq UGW Software $(UGW_VERSION) on $(CPE_NAME) CPE

ifeq ($(CONFIG_PACKAGE_kmod-ltqcpe_mlcnand),y)
  UBINIZE_OPTS:= -p $(patsubst %k,%KiB,$(JFFS2_BLOCKSIZE)) -m 8192 -s 8192
else
  UBINIZE_OPTS:= -p $(patsubst %k,%KiB,$(JFFS2_BLOCKSIZE)) -m 2048 -s 512
endif

define put_ltq_banner
	b_str=" | $(UGW_BANNER) |"; \
	b_chars=`echo -n $$b_str | wc -m`; \
	(echo -n " +"; for ((i=2;i<$$b_chars;i++)); do echo -n "-"; done; echo "+"; \
	echo "$$b_str";echo -n " +";for ((i=2;i<$$b_chars;i++));do echo -n "-";done;echo -en "+\n\n") > $(1)
endef

# add_image_padding <input_image_name> <block_size(bytes)> <size_to_less(bytes)>
# Result: <input_image_name>.padded
define add_image_padding
	len=`wc -c $(1) | awk '{ printf $$1 }'`; \
	pad=`expr $(2) - $$len % $(2)`; \
	pad=`expr $$pad % $(2)`; \
	pad=`expr $$pad - $(3)`; [ $$pad -lt 0 ] && pad=0; \
	cat $(1) > $(1).padded; \
	dd if=/dev/zero of=$(1).padded bs=1 count=$$pad seek=$$len
endef

# Function to convert Kb/Mb/Gb to bytes
define conv_bytes
	_flg=$$(echo $(1) | sed 's/[0-9]//g'); \
	_num=$$(echo $(1) | sed 's/[a-zA-Z]//g'); \
	case $$_flg in \
		"k"|"K") echo $$((_num*1024));; \
		"m"|"M") echo $$((_num*1024*1024));; \
		"g"|"G") echo $$((_num*1024*1024*1024));; \
		*) echo $$_num;; \
	esac
endef

# Function to strip config macros from /etc/config.sh. Reference file used: tmp/ifx_config.sh
# Scan path: etc/ opt/ ramdisk_copy/ bin/ usr/ lib/
define strip_config_macros
	if [ -f $(TMP_DIR)/ifx_config.sh ]; then \
		scan_dirs="etc/ opt/ ramdisk_copy/ bin/ usr/ lib/"; \
		rm -rf $(TMP_DIR)/stripmacros; mkdir -p $(TMP_DIR)/stripmacros; \
		echo -en "\nStripping config macros from 'etc/config.sh'\nScanning: $$scan_dirs\n"; \
		echo -en "#\n# Automatically generated config for system scripts. Don't edit.\n#\n" \
			> $(TMP_DIR)/stripmacros/config.sh; \
		tot_macros=$$(grep export $(TMP_DIR)/ifx_config.sh | wc -l); \
		found=0; inc=0; cd $(TARGET_DIR)/; \
		fls="$$(find $$scan_dirs -type f | xargs grep -lI 'CONFIG_' | grep -v "etc\/config.sh" | sort -u)"; \
		grep export $(TMP_DIR)/ifx_config.sh | \
		  ( while read cfg_macro; do \
			cfgm=$$(echo $$cfg_macro | sed -e 's/export[ ]*//g' -e 's/=.*//g'); \
			val="$$(grep -wl $$cfgm $$fls)"; \
			if [ -n "$$val" ]; then \
				found=$$(expr $$found + 1); \
				echo "$$cfg_macro" >> $(TMP_DIR)/stripmacros/config.sh; \
			fi; inc=$$(expr $$inc + 1); \
			echo -en "Processing: $$inc/$$tot_macros macros. Used macros: $$found\r"; \
		  done ); echo -en "\n\n"; \
		if [ -f $(TMP_DIR)/stripmacros/config.sh ]; then \
		$(INSTALL_BIN) $(TMP_DIR)/stripmacros/config.sh $(TARGET_DIR)/etc/config.sh; fi; \
	else echo -en "\nERROR: Unable to find $(TMP_DIR)/ifx_config.sh for macro stripping!!\n\n"; fi
endef

define Image/BuildKernel
	$(STAGING_DIR_HOST)/bin/lzma e $(KDIR)/vmlinux $(KDIR)/vmlinux.lzma
	mkimage -A mips -O linux -T kernel -a 0x80002000 -C lzma -e \
		0x80002000 \
		-n 'MIPS OpenWrt Linux-$(LINUX_VERSION)' \
		-d $(KDIR)/vmlinux.lzma $(KDIR)/uImage

	cp -f $(KDIR)/uImage $(BIN_DIR)/openwrt-$(BOARD)-uImage

	$$(call add_image_padding,$(KDIR)/vmlinux.lzma,\
		$(if $(JFFS2_BLOCKSIZE),$(shell expr $(patsubst %k,%,$(JFFS2_BLOCKSIZE)) \* 1024),16),\
		$(if $(JFFS2_BLOCKSIZE),64,0))
	mkimage -A mips -O linux -T kernel -a \
		0x$(shell grep -w _text $(KDIR)/linux-$(LINUX_VERSION)/System.map 2>/dev/null| awk '{ printf "%s", $$1 }') -C lzma -e \
		0x$(shell grep -w kernel_entry $(KDIR)/linux-$(LINUX_VERSION)/System.map 2>/dev/null| awk '{ printf "%s", $$1 }') \
		-n 'MIPS LTQCPE Linux-$(LINUX_VERSION)' \
		-d $(KDIR)/vmlinux.lzma.padded $(KDIR)/uImage.padded

	mkdir -p $(TARGET_IMG_DIR)
	cp -f $(KDIR)/uImage.padded $(TARGET_IMG_DIR)/uImage
endef

define Image/mkfs/prepare/platform
	- echo "$(UGW_VERSION)" > $(TARGET_DIR)/etc/version
	- date +%H%M-%d%b%y > $(TARGET_DIR)/etc/timestamp
	- @$$(call put_ltq_banner,$(TARGET_DIR)/etc/banner)
	- $(if $(wildcard $(TOPDIR)/patches.list), install -m644 $(TOPDIR)/patches.list $(TARGET_DIR)/etc/)
	- rm -rf $(TARGET_DIR)/tmp
	- cd $(TARGET_DIR); ln -sf ramdisk/tmp tmp
	- $(CP) $(TMP_DIR)/ifx_config.sh $(TARGET_DIR)/etc/config.sh
	- $(if $(CONFIG_EXTERNAL_TOOLCHAIN),$(CP) $(TOOLCHAIN_ROOT_DIR)/lib/librt-*.so $(TOOLCHAIN_ROOT_DIR)/lib/libutil-*.so $(TARGET_DIR)/lib, \
	$(CP) $(TOOLCHAIN_DIR)/lib/librt-*.so $(TOOLCHAIN_DIR)/lib/libutil-*.so $(TARGET_DIR)/lib)
	@$$(call strip_config_macros)
endef

define Image/Build/squashfs
	mkdir -p $(TARGET_DIR)/overlay
	$(if $(CONFIG_TARGET_UBI_MTD_SUPPORT),
		mkdir -p $(TARGET_DIR)/mnt/data
		$(if $(CONFIG_TARGET_DATAFS_JFFS2),
			echo jffs2:$(CONFIG_TARGET_ROOTFS_DATAFS_SIZE) > $(TARGET_DIR)/mnt/data/fs)
		$(if $(CONFIG_TARGET_DATAFS_UBIFS),
			echo ubifs:$(CONFIG_TARGET_ROOTFS_DATAFS_SIZE) > $(TARGET_DIR)/mnt/data/fs)
	)
	$(MKSQUASHFS_CMD) $(TARGET_DIR) $(KDIR)/root.squashfs $(if $(CONFIG_TARGET_UBI_MTD_SUPPORT),,-nopad) \
		-noappend -root-owned $(if $(CONFIG_LINUX_2_6),$(SQUASHFS_OPTS),-comp xz -Xpreset 9 -Xe -Xlc 0 -Xlp 2 -Xpb 2 -processors 1)
	$(if $(CONFIG_TARGET_UBI_MTD_SUPPORT),,$$(call add_image_padding,$(KDIR)/root.$(1),16,0))
	mkimage -A MIPS -O Linux -C lzma -T filesystem -e 0x00 -a 0x00 -n "LTQCPE RootFS" \
		-d $(if $(CONFIG_TARGET_UBI_MTD_SUPPORT),$(KDIR)/root.$(1),$(KDIR)/root.$(1).padded) $(KDIR)/rootfs.img.padded
	mkdir -p $(TARGET_IMG_DIR)
	cp -f $(KDIR)/rootfs.img.padded $(TARGET_IMG_DIR)/rootfs.img
endef

define Image/Build/jffs2-$(JFFS2_BLOCKSIZE)
	mkdir -p $(TARGET_IMG_DIR)
	$(if $(CONFIG_TARGET_UBI_MTD_SUPPORT),
	    $(eval jffs2_blk_size:=$(shell echo $$(($(patsubst %k,%,$(JFFS2_BLOCKSIZE))-$(patsubst %k,%,$(JFFS2_BLOCKSIZE))/64))KiB))
	,
	    $(eval jffs2_blk_size:=$(patsubst %k,%KiB,$(JFFS2_BLOCKSIZE))))
	$(STAGING_DIR_HOST)/bin/mkfs.jffs2 -n -p $(JFFS2OPTS) \
		-e $(jffs2_blk_size) \
		-o $(KDIR)/root.$(1) \
		-d $(TARGET_DIR) 2>&1 1>/dev/null | awk '/^.+$$$$/'
	$(if $(CONFIG_TARGET_UBI_MTD_SUPPORT),
		cp -f $(KDIR)/root.$(1) $(TARGET_IMG_DIR)/rootfs.img
	,
		mkimage -A MIPS -O Linux -C gzip -T filesystem -e 0x00 -a 0x00 -n "LTQCPE RootFS" \
			-d $(KDIR)/root.$(1) \
			$(TARGET_IMG_DIR)/rootfs.img
	)
endef

define Image/Build
	$(call Image/Build/$(1),$(1))
endef

define Image/Build/firmware
	FIRMWARE_DIR_NAME="firmware"; \
	GET_FIRMWARE_PATH="$(STAGING_DIR)"; \
	PUT_FIRMWARE_PATH="$(TARGET_IMG_DIR)"; \
	OUT_FIRMWARE_NAME="firmware.img"; \
	\
	PLATFORM=`echo $(SUBTARGET)|cut -d_ -f2-|awk '{ print toupper($$1) }'`; \
	if [ -n "$(CONFIG_PACKAGE_open_uboot)" -a -z "$(CONFIG_UBOOT_CONFIG_FIRMWARE_IN_ROOTFS)" ]; then \
	   if [ -d $$GET_FIRMWARE_PATH/$$FIRMWARE_DIR_NAME ]; then \
		rm -rf $$PUT_FIRMWARE_PATH/$$OUT_FIRMWARE_NAME; \
		cd $$GET_FIRMWARE_PATH; rm -f $(KDIR)/firmware.squashfs; \
		$(MKSQUASHFS_CMD) $$FIRMWARE_DIR_NAME $(KDIR)/firmware.squashfs \
			$(if $(CONFIG_LINUX_2_6),$(SQUASHFS_OPTS),-comp xz -Xpreset 9 -Xe -Xlc 0 -Xlp 2 -Xpb 2 -processors 1); \
		mkdir -p $(TARGET_IMG_DIR); \
		mkimage -A MIPS -O Linux -C lzma -T $$FIRMWARE_DIR_NAME -e 0x00 -a 0x00 -n \
			"$$PLATFORM Firmware" -d $(KDIR)/firmware.squashfs $$OUT_FIRMWARE_NAME; \
		mkdir -p $$PUT_FIRMWARE_PATH; \
		mv $$OUT_FIRMWARE_NAME $$PUT_FIRMWARE_PATH/; \
		chmod 644 $$PUT_FIRMWARE_PATH/$$OUT_FIRMWARE_NAME; \
	   else \
		echo "$$GET_FIRMWARE_PATH/$$FIRMWARE_DIR_NAME not found..!"; \
		exit 1; \
	   fi; \
	fi;
endef

define Image/Build/fullimage
	echo -en "\n\033[32;02mCreating fullimage.img\033[00m\n"; \
	IMAGE_LIST="$(TARGET_IMG_DIR)/rootfs.img \
		$(TARGET_IMG_DIR)/uImage"; \
	if [ -n "$(CONFIG_PACKAGE_open_uboot)" -a -z "$(CONFIG_UBOOT_CONFIG_FIRMWARE_IN_ROOTFS)" ]; then \
		IMAGE_LIST="$$IMAGE_LIST $(TARGET_IMG_DIR)/firmware.img"; \
	fi; \
	ONEIMAGE="$(TARGET_IMG_DIR)/fullimage.img"; \
	PLATFORM=`echo $(SUBTARGET)|cut -d_ -f2-|awk '{ print toupper($$1) }'`; \
	rm -f $$ONEIMAGE; \
	for i in $$IMAGE_LIST; do \
		if [ -e $$i  ] ; then \
			len=`wc -c $$i | awk '{ printf $$1 }'`; \
			pad=`expr 16 - $$len % 16`; \
			pad=`expr $$pad % 16`; \
			if [ -e $$ONEIMAGE.tmp ] ; then \
				cat $$i >> $$ONEIMAGE.tmp; \
			else \
				cat $$i > $$ONEIMAGE.tmp; \
			fi; \
			while [ $$pad -ne 0 ]; do \
				echo -n "0" >> $$ONEIMAGE.tmp; \
				pad=`expr $$pad - 1`; \
			done; \
		else \
			echo "$$i not found!"; \
			rm -f $$ONEIMAGE.tmp; \
			exit 1; \
		fi; \
	done; \
	mkimage -A MIPS -O Linux -C none -T multi -e 0x00 -a 0x00 -n \
		"$$PLATFORM Fullimage" -d $$ONEIMAGE.tmp $$ONEIMAGE; \
	rm -f $$ONEIMAGE.tmp; \
	chmod 644 $$ONEIMAGE;
endef

define Image/Build/system_sw
	if [ -n "$(CONFIG_UBOOT_CONFIG_BOOT_FROM_NAND)" -a -n "$(CONFIG_TARGET_UBI_MTD_SUPPORT)" ]; then \
		echo -en "\n\033[32;02mCreating UBI image system_sw.bin\033[00m\n"; \
		OUT_DIR="$(TARGET_IMG_DIR)/flashprogram_bins"; mkdir -p $$OUT_DIR; \
		if [ -z "$(CONFIG_UBOOT_CONFIG_FIRMWARE_IN_ROOTFS)" ]; then \
			$(CP) ./ubinize_fw.cfg $(KDIR)/ubinize.cfg; \
		else $(CP) ./ubinize.cfg $(KDIR)/; fi; \
		(cd $(KDIR)/ && \
			$(STAGING_DIR_HOST)/bin/ubinize $(UBINIZE_OPTS) \
			-o $$OUT_DIR/system_sw.bin ubinize.cfg); \
	fi;
endef

define Image/Build/totalimage
	if [ -n "$(CONFIG_PACKAGE_open_uboot)" ]; then \
	  echo -en "\n\033[32;02mCreating totalimage.img\033[00m\n"; \
	  PLATFORM=`echo $(SUBTARGET)|cut -d_ -f2-|awk '{ print toupper($$1) }'`; \
	  if [ -n "$(CONFIG_UBOOT_CONFIG_BOOT_FROM_NAND)" ]; then \
		UBOOT_IMG="$(TARGET_IMG_DIR)/u-boot-nand.bin"; \
	  else UBOOT_IMG="$(TARGET_IMG_DIR)/u-boot.ltq"; fi; \
	  $(KDIR)/u-boot-*/tools/mkimage -A MIPS -T uboot -C lzma -a 0x00 -e 0x00 -n 'U-Boot Img' \
		-d $$UBOOT_IMG $(KDIR)/u-bootimg.ltq || exit 1; \
	  cat $(KDIR)/u-bootimg.ltq > $(KDIR)/totalimage.tmp || exit 1; \
	  if [ -z "$(CONFIG_UBOOT_CONFIG_VR9_GPHY_FW_EMBEDDED)" -a -n "$(CONFIG_PACKAGE_ltq-gphy-firmware)" ]; then \
		cp -f $(TARGET_IMG_DIR)/gphy_firmware.img $(KDIR)/; \
		$(if $(JFFS2_BLOCKSIZE),padd_blksize=$(shell expr $(patsubst %k,%,$(JFFS2_BLOCKSIZE)) \* 1024),padd_blksize=16384); \
		if [ $$padd_blksize -gt 262144 ]; then padd_blksize="262144"; fi; \
		$(KDIR)/u-boot-*/scripts_platform/pad2align.sh -n $$padd_blksize $(KDIR)/gphy_firmware.img; \
		$(KDIR)/u-boot-*/tools/mkimage -A mips -T firmware -C lzma -a 0x00 -e 0x00 \
			-n "gphyfw" -d $(KDIR)/gphy_firmware.img $(KDIR)/gphy_firmwareimg.img || exit 1; \
		cat $(KDIR)/gphy_firmwareimg.img >> $(KDIR)/totalimage.tmp || exit 1; \
	  fi; echo "Adding fullimage.."; \
	  cat $(TARGET_IMG_DIR)/rootfs.img >> $(KDIR)/totalimage.tmp || exit 1; \
	  cat $(TARGET_IMG_DIR)/uImage >> $(KDIR)/totalimage.tmp || exit 1; \
	  if [ -z "$(CONFIG_UBOOT_CONFIG_FIRMWARE_IN_ROOTFS)" ]; then \
		cat $(TARGET_IMG_DIR)/firmware.img >> $(KDIR)/totalimage.tmp || exit 1; \
	  fi; \
	  $(KDIR)/u-boot-*/tools/mkimage -A MIPS -O Linux -C none -T multi -e 0x00 -a 0x00 -n \
		"$$PLATFORM Totalimage" -d $(KDIR)/totalimage.tmp \
		$(TARGET_IMG_DIR)/totalimage.img || exit 1; \
	  chmod 644 $(TARGET_IMG_DIR)/totalimage.img; \
	fi
endef

define Image/Build/flashprogrammer
	if [ -n "$(CONFIG_TARGET_UBI_MTD_SUPPORT)" ]; then \
	  echo -en "\n\033[32;02mCreating binaries for flashprogrammer\033[00m\n"; \
	  MODEL_PREFIX=`echo $(CONFIG_IFX_MODEL_NAME)|cut -d_ -f1`; \
	  OUT_DIR="$(TARGET_IMG_DIR)/flashprogram_bins"; mkdir -p $$OUT_DIR; \
	  OUT_BIN="$$OUT_DIR/flashimage-$$MODEL_PREFIX-NAND-$(JFFS2_BLOCKSIZE).bin"; \
	  mtdparts="`echo $(CONFIG_UBOOT_CONFIG_MTDPARTS)|cut -d: -f2-|sed 's/,/ /g'`"; \
	  cd $$OUT_DIR/ && ( \
		rm -f $$OUT_BIN; \
		for mtdb in $$mtdparts; do \
			echo $$mtdb; mtd_info=(`echo $$mtdb|sed 's/[()]/ /g'`); \
			m_byte=`$(call conv_bytes,$${mtd_info[0]})`; \
			case $${mtd_info[1]} in \
			   "uboot") ln -sf ../u-boot-nand.bin u-boot-nand.bin; \
				$(call add_image_padding,u-boot-nand.bin,$$m_byte,0); \
				cat u-boot-nand.bin.padded >> $$OUT_BIN;; \
			   "uboot+gphyfw") echo "uboot"; ln -sf ../u-boot-nand.bin u-boot-nand.bin; \
				$(call add_image_padding,u-boot-nand.bin,$$(($$m_byte/2)),0); \
				cat u-boot-nand.bin.padded >> $$OUT_BIN; \
				echo "gphy_firmware"; ln -sf ../gphy_firmware.img gphy_firmware.bin; \
				$(call add_image_padding,gphy_firmware.bin,$$(($$m_byte/2)),0); \
				cat gphy_firmware.bin.padded >> $$OUT_BIN;; \
			   "ubootconfig"*) ln -sf ../ubootenv.img ubootenv.bin; \
				$(call add_image_padding,ubootenv.bin,$$m_byte,0); \
				cat ubootenv.bin.padded >> $$OUT_BIN;; \
			   "gphyfirmware") ln -sf ../gphy_firmware.img gphy_firmware.bin; \
				$(call add_image_padding,gphy_firmware.bin,$$m_byte,0); \
				cat gphy_firmware.bin.padded >> $$OUT_BIN;; \
			   "system_sw") cat system_sw.bin >> $$OUT_BIN;; \
			esac; rm -f *.padded; \
		done;); \
	fi
endef

ifeq ($(CONFIG_TARGET_ROOTFS_INITRAMFS),)

ifneq ($(CONFIG_TARGET_ltqcpe),)
  define Image/mkfs/firmware
	$(call Image/Build,firmware)
  endef

  define Image/mkfs/fullimage
	$(call Image/Build,fullimage)
	$(call Image/Build,system_sw)
	$(call Image/Build,totalimage)
	$(call Image/Build,flashprogrammer)
  endef

  define Image/mkfs/platform
	$$(call Image/Build,firmware)
	$$(call Image/Build,fullimage)
	$$(call Image/Build,system_sw)
	$$(call Image/Build,totalimage)
	$$(call Image/Build,flashprogrammer)
  endef
endif
endif

define BuildImage_platform
rootfs:
	$(call Image/mkfs/prepare)
	$(call Image/mkfs/jffs2)
	$(call Image/mkfs/squashfs)
	$(call Image/mkfs/tgz)
	$(call Image/mkfs/cpiogz)
	$(call Image/mkfs/ext2)
	$(call Image/mkfs/iso)
	$(call Image/mkfs/ubifs)
	$(call Image/Checksum)

firmware:
	$$(call Image/mkfs/firmware)

fullimage:
	$$(call Image/mkfs/fullimage)

endef

$(eval $(call BuildImage))
$(eval $(call BuildImage_platform))
