#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-dsl-ar9-firmware-adsl-b
PKG_VERSION:=04.05.01.0A.00.02
PKG_MD5SUM:=9a6d222c95ed35f246a830f6ff6ba28a
PKG_BASE_NAME:=dsl_ar9_firmware_adsl_b
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_UNPACK:=gzip -dc $(DL_DIR)/$(PKG_SOURCE) | $(TAR) -C $(PKG_BUILD_DIR) $(TAR_OPTIONS)
PKG_SOURCE_URL:=http://localhost
PKG_TARGET_BASE_NAME:=dsl_firmware_b.bin

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

FIRMWARE_DIR:=/lib/firmware/$(LINUX_VERSION)

ifeq ($(DUMP),)
  $(info "FIRMWARE_DIR = $(FIRMWARE_DIR)")
   STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell ( \
      grep '^CONFIG_UBOOT_CONFIG_FIRMWARE_IN_ROOTFS' $(TOPDIR)/.config; \
      grep '^CONFIG_$(PKG_NAME)' $(TOPDIR)/.config) | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Lantiq
  TITLE:=AR100 (Annex B) firmware
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	ARX100 (Annex B) firmware
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/$(FIRMWARE_DIR)
	$(if $(CONFIG_UBOOT_CONFIG_FIRMWARE_IN_ROOTFS), \
		$(CP) $(PKG_BUILD_DIR)/*.bin $(1)/$(FIRMWARE_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION).bin; \
		$(CP) $(PKG_BUILD_DIR)/LICENSE $(1)/$(FIRMWARE_DIR)/; \
		ln -sf $(PKG_BASE_NAME)-$(PKG_VERSION).bin $(1)/$(FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME); \
	,\
		rm -rf $(STAGING_DIR)/firmware/; mkdir -p $(STAGING_DIR)/firmware/; \
		$(CP) $(PKG_BUILD_DIR)/*.bin $(STAGING_DIR)/firmware/$(PKG_BASE_NAME)-$(PKG_VERSION).bin; \
		$(CP) $(PKG_BUILD_DIR)/LICENSE $(STAGING_DIR)/firmware/; \
		ln -sf $(PKG_BASE_NAME)-$(PKG_VERSION).bin $(STAGING_DIR)/firmware/$(PKG_TARGET_BASE_NAME); \
	)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
