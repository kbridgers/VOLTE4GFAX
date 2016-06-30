#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
# 

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-ethsw
PKG_VERSION:=0.9.0
PKG_RELEASE:=1
#PKG_SOURCE:=$(PKG_NAME)-utilities-$(PKG_VERSION).tar.gz
#PKG_SOURCE_URL:=http://localhost
#PKG_CAT:=zcat
PKG_SOURCE_DIR:=$(BUILD_DIR)/$(PKG_NAME)-utilities-$(PKG_VERSION)
PKG_BUILD_DIR:=$(PKG_SOURCE_DIR)
#PKG_UNPACK += cp -f $(PKG_SOURCE_DIR)/include/ifx_ethsw.h $(LINUX_DIR)/include/asm/ifx/
IFX_BIN_DIR=opt/ifx/bin
#PKG_START_SCRIPT:=ifx_load_switch.sh

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=Ethernet Switch Utility
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq GmbH
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	Ethernet Switch driver with ioctl support
endef

### Menuconfig ##################################
define Package/$(PKG_NAME)/config
        menu "$(PKG_NAME) features"
        depends on PACKAGE_$(PKG_NAME)
        source "$(SOURCE)/config/Config.in"
        endmenu
endef

### Configurations depends on Menuconfig ########
IFX_CFLAGS_y:=
IFX_CFLAGS_y +=$(TARGET_CFLAGS) -Wall -Werror

ifeq ($(CONFIG_VR9),y)
IFX_CFLAGS_y += -DVR9
endif
ifeq ($(CONFIG_AR10),y)
IFX_CFLAGS_y += -DAR10
endif
ifeq ($(CONFIG_HN1),y)
IFX_CFLAGS_y += -DHN1
endif
ifeq ($(CONFIG_AR9),y)
IFX_CFLAGS_y += -DAR9
endif

ifeq ($(CONFIG_DANUBE),y)
IFX_CFLAGS_y += -DDANUBE
endif

ifeq ($(CONFIG_AMAZON_SE),y)
IFX_CFLAGS_y += -DAMAZON_SE
endif

IFX_CFLAGS:=$(IFX_CFLAGS_y)

CONFIGURE_ARGS += --enable-kernelincl="$(LINUX_DIR)/include"

ifeq ($(CONFIG_LINUX_2_6),y)
CONFIGURE_ARGS += --enable-linux-26 --enable-kernelbuild="$(LINUX_DIR)" ARCH=$(LINUX_KARCH)
PKG_MODULE_EXTENSION=.ko
endif

#### Target Rules ###############################
define Build/Prepare
	#$(call Build/Prepare/Default)
	rm -rf $(PKG_BUILD_DIR)
	mkdir $(PKG_BUILD_DIR)
	cp -r ./files/* $(PKG_BUILD_DIR)
	cp -f $(PKG_SOURCE_DIR)/include/ifx_ethsw.h $(LINUX_DIR)/include/asm/ifx/
endef

define Build/Compile
	cd $(PKG_BUILD_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) IFX_CFLAGS="$(IFX_CFLAGS)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/switch_utility $(1)/usr/bin/
ifeq ($(CONFIG_AR9),y)
	touch $(PKG_BUILD_DIR)/switch_utility_int
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/switch_utility_int $(1)/usr/bin/
endif
	$(INSTALL_DIR) $(1)/ramdisk_copy/tmp/
	$(RM) $(1)/ramdisk_copy/tmp/busybox
	$(RM) $(1)/ramdisk_copy/tmp/libifx_httpd.so
	$(RM) $(1)/ramdisk_copy/tmp/gphy
	$(INSTALL_DIR) $(1)/etc/rc.d/
#	$(RM) $(1)/etc/rc.d/S81ifx_load_switch.sh
	$(RM) $(1)/etc/init.d/ifx_load_switch.sh
	
	if [ A$(CONFIG_FEATURE_IFX_GPHY_WEB_GUI) = Ay ]; then \
		$(INSTALL_DIR) $(1)/ramdisk_copy/tmp/; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/web_src/busybox $(1)/ramdisk_copy/tmp/; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/web_src/libifx_httpd.so $(1)/ramdisk_copy/tmp/; \
		mkdir -p $(1)/ramdisk_copy/tmp/gphy; \
		$(CP) $(PKG_BUILD_DIR)/web_src/gphy/* $(1)/ramdisk_copy/tmp/gphy/; \
		$(INSTALL_DIR) $(1)/etc/rc.d/; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/web_src/S81ifx_load_switch.sh $(1)/etc/rc.d/; \
	fi; \

endef

define Build/clean
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
