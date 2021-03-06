#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=ifx-voice-cpe-vmmc-firmware-ar9
PKG_VERSION:=5.0.0.4.0
PKG_MD5SUM:=f227d8e080b1db28ecc863fce17da7b8
PKG_BASE_NAME:=fw_voip_ar9

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_UNPACK:=gzip -dc $(DL_DIR)/$(PKG_SOURCE) | $(TAR) -C $(PKG_BUILD_DIR) $(TAR_OPTIONS)
PKG_SOURCE_URL:=http://localhost
PKG_TARGET_BASE_NAME:=ar9_firmware.bin
IFX_FIRMWARE_DIR=/opt/ifx/downloads
LANTIQ_FIRMWARE_DIR=/lib/firmware

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Lantiq
  TITLE:=AR9 voice firmware
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=ifx-voice-cpe-tapi @LTQ_VOICE_CPE_VMMC_WITH_DEVICE_AR9
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	AR9 voice firmware.
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/$(IFX_FIRMWARE_DIR)
	$(INSTALL_DIR) $(1)/$(LANTIQ_FIRMWARE_DIR)
	$(CP) $(PKG_BUILD_DIR)/*.bin $(1)/$(LANTIQ_FIRMWARE_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION).bin
	$(CP) $(PKG_BUILD_DIR)/LICENSE $(1)/$(LANTIQ_FIRMWARE_DIR)/
	ln -snf $(LANTIQ_FIRMWARE_DIR)/LICENSE $(1)/$(IFX_FIRMWARE_DIR)/LICENSE
	ln -sf $(PKG_BASE_NAME)-$(PKG_VERSION).bin $(1)/$(LANTIQ_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME)
	ln -sf $(LANTIQ_FIRMWARE_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION).bin $(1)/$(IFX_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
