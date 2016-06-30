#
# LTQCPE rootfs addon for OpenWRT basefiles
#

ifneq ($(CONFIG_EXTERNAL_TOOLCHAIN),)
  define Package/libc/install_lib
	$(CP) $(filter-out %/libdl_pic.a %/libpthread_pic.a %/libresolv_pic.a %/libnsl_pic.a,$(wildcard $(TOOLCHAIN_ROOT_DIR)/usr/lib/lib*.a) $(wildcard $(TOOLCHAIN_ROOT_DIR)/lib/lib*.a)) $(1)/lib/
	$(if $(wildcard $(TOOLCHAIN_ROOT_DIR)/usr/lib/libc_so.a),$(CP) $(TOOLCHAIN_ROOT_DIR)/usr/lib/libc_so.a $(1)/lib/libc_pic.a)
	$(if $(wildcard $(TOOLCHAIN_ROOT_DIR)/usr/lib/gcc/*/*/libgcc.map), \
		$(CP) $(TOOLCHAIN_ROOT_DIR)/usr/lib/gcc/*/*/libgcc_pic.a $(1)/lib/libgcc_s_pic.a; \
		$(CP) $(TOOLCHAIN_ROOT_DIR)/usr/lib/gcc/*/*/libgcc.map $(1)/lib/libgcc_s_pic.map \
	)
  endef
  define Package/libpthread/install_lib
	$(if $(wildcard $(TOOLCHAIN_ROOT_DIR)/usr/lib/libpthread_so.a),$(CP) $(TOOLCHAIN_ROOT_DIR)/usr/lib/libpthread_so.a $(1)/lib/libpthread_pic.a)
  endef
else
  define Package/libc/install_lib
	$(CP) $(filter-out %/libdl_pic.a %/libpthread_pic.a %/libresolv_pic.a %/libnsl_pic.a,$(wildcard $(TOOLCHAIN_DIR)/usr/lib/lib*.a)) $(1)/lib/
	$(if $(wildcard $(TOOLCHAIN_DIR)/usr/lib/libc_so.a),$(CP) $(TOOLCHAIN_DIR)/usr/lib/libc_so.a $(1)/lib/libc_pic.a)
	$(if $(wildcard $(TOOLCHAIN_DIR)/usr/lib/gcc/*/*/libgcc.map), \
		$(CP) $(TOOLCHAIN_DIR)/usr/lib/gcc/*/*/libgcc_pic.a $(1)/lib/libgcc_s_pic.a; \
		$(CP) $(TOOLCHAIN_DIR)/usr/lib/gcc/*/*/libgcc.map $(1)/lib/libgcc_s_pic.map \
	)
  endef
endif

define Package/base-files/install-target
	rm -f $(1)/var
	rm -f $(1)/etc/{passwd,hosts,fstab,services}
	rm -rf $(1)/tmp
	rm -rf $(1)/etc/init.d

	mkdir -p $(1)/{usr/bin,usr/lib,etc/dsl_api,etc/ppp,etc/init.d,etc/rc.d,proc,root,ramdisk,ramdisk_copy,mnt/overlay}
	mkdir -p $(1)/ramdisk/{tmp,var,flash}
	mkdir -p $(1)/ramdisk_copy/{var/tmp,var/run,var/log,var/lock,etc/Wireless,etc/dnrd,etc/ppp/peers,tftp_upload,usb}

	if [ -d $(PLATFORM_DIR)/$(PROFILE)/base-files/etc/init.d/. ]; then \
		$(CP) $(PLATFORM_DIR)/$(PROFILE)/base-files/etc/init.d/* $(1)/etc/init.d/; \
	fi
	$(if $(filter-out $(PLATFORM_DIR),$(PLATFORM_SUBDIR)), \
		if [ -d $(PLATFORM_SUBDIR)/base-files/etc/init.d/. ]; then \
			$(CP) $(PLATFORM_SUBDIR)/base-files/etc/init.d/* $(1)/etc/init.d/; \
		fi \
	)
	-sed -i -e '/\/etc\/passwd/d' -e '/\/etc\/hosts/d' $(1)/CONTROL/conffiles

	$(if $(wildcard $(PLATFORM_SUBDIR)/base-files/etc/fstab), \
		cp -f $(PLATFORM_SUBDIR)/base-files/etc/fstab $(1)/etc/; \
	,\
		cp -f $(PLATFORM_DIR)/base-files/etc/fstab $(1)/etc/; \
	)

	$(if $(CONFIG_TARGET_ltqcpe_platform_ar10_vrx318), \
		if [ -d $(PLATFORM_SUBDIR)/base-files-vrx318/ ]; then \
			$(CP) $(PLATFORM_SUBDIR)/base-files-vrx318/* $(1)/; \
		fi \
	)

	$(if $(CONFIG_TARGET_UBI_MTD_SUPPORT), \
		if [ -d $(PLATFORM_DIR)/base-files-ubi/. ]; then \
			$(CP) $(PLATFORM_DIR)/base-files-ubi/* $(1)/; \
			mkdir -p $(1)/mnt/data; \
			$(if $(CONFIG_TARGET_DATAFS_JFFS2), \
				echo jffs2 > $(1)/mnt/data/fs;) \
			$(if $(CONFIG_TARGET_DATAFS_UBIFS), \
				echo ubifs > $(1)/mnt/data/fs;) \
		fi \
	)

	$(if $(CONFIG_LINUX_3_5-rc2), \
		if [ -d $(PLATFORM_DIR)/base-files-3.5/. ]; then \
			$(CP) $(PLATFORM_DIR)/base-files-3.5/* $(1)/; \
		fi \
	)

	cd $(1); \
		mkdir -p lib/firmware/$(LINUX_VERSION); \
		ln -sf lib/firmware/$(LINUX_VERSION) firmware; \
		ln -sf ramdisk/var var; \
		ln -sf ramdisk/flash flash

	mkdir -p $(STAGING_DIR)/usr/include/
	$(if $(CONFIG_PACKAGE_ifx-utilities),cp -f $(PLATFORM_DIR)/base-files/etc/rc.conf $(STAGING_DIR)/usr/include/)

	mkdir -p $(1)/etc
	cd $(1)/etc; \
		rm -f rc.conf; \
		ln -sf ../ramdisk/etc/dnrd dnrd; \
		ln -sf ../ramdisk/etc/hostapd.conf hostapd.conf; \
		ln -sf ../ramdisk/etc/hosts hosts; \
		ln -sf ../ramdisk/etc/ilmid ilmid; \
		ln -sf ../ramdisk/etc/inetd.conf inetd.conf; \
		ln -sf ../ramdisk/etc/ipsec.conf ipsec.conf; \
		ln -sf ../ramdisk/etc/ipsec.secrets ipsec.secrets; \
		ln -sf ../proc/mounts mtab; \
		ln -sf ../flash/passwd passwd; \
		ln -sf ../ramdisk/flash/rc.conf rc.conf; \
		ln -sf ../ramdisk/etc/resolv.conf resolv.conf; \
		ln -sf ../ramdisk/etc/ripd.conf ripd.conf; \
		ln -sf ../ramdisk/etc/snmp snmp; \
		ln -sf ../ramdisk/etc/tr69 tr69; \
		ln -sf ../ramdisk/etc/udhcpd.conf udhcpd.conf; \
		ln -sf ../ramdisk/etc/zebra.conf zebra.conf; \
		ln -sf ../ramdisk/etc/TZ TZ; \
		ln -sf ../ramdisk/etc/services services

	mkdir -p $(1)/etc/ppp
	cd $(1)/etc/ppp; \
		ln -sf ../../ramdisk/etc/ppp/options options; \
		ln -sf ../../ramdisk/etc/ppp/peers peers; \
		ln -sf ../../ramdisk/etc/ppp/resolv.conf resolv.conf

	mkdir -p $(1)/etc/rc.d
	cd $(1)/etc/rc.d; \
		ln -sf ../init.d init.d; \
		ln -sf ../config.sh config.sh

	mkdir -p $(1)/mnt
	cd $(1)/mnt; \
		ln -sf ../ramdisk/usb usb

	$(if $(CONFIG_UBOOT_CONFIG_BOOT_FROM_NAND),mkdir -p $(1)/etc/sysconfig)
endef
