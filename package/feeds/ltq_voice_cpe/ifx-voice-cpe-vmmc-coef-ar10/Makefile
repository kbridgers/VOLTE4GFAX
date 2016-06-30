#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=ifx-voice-cpe-vmmc-coef-xRX300
PKG_VERSION:=0.1.0
PKG_BASE_NAME:=coef_voip_xrx300

# IMPORTANT - include the version-override here after PKG_VERSION definition
include $(INCLUDE_DIR)/package-version-override.mk
# IMPORTANT - ... and before using PKG_VERSION below

PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_UNPACK:=gzip -dc $(DL_DIR)/$(PKG_SOURCE) | $(TAR) -C $(PKG_BUILD_DIR) $(TAR_OPTIONS)
PKG_SOURCE_URL:=http://localhost
PKG_TARGET_BASE_NAME_FXS:=xrx300_bbd_fxs.bin
PKG_TARGET_BASE_NAME_FXO:=xrx300_bbd_fxo.bin
IFX_FIRMWARE_DIR=/opt/ifx/downloads
LANTIQ_FIRMWARE_DIR=/lib/firmware

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Lantiq
  TITLE:=xRX300 family voice coefficient samples
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=ifx-voice-cpe-tapi @LTQ_VOICE_CPE_VMMC_WITH_DEVICE_XRX300
  MENU:=1
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
	source "$(SOURCE)/config/Config.in"
endef

# FXS part
ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_XRX300_ETSI),y)
	PKG_ORGINAL_BASE_NAME_FXS:=$(PKG_BUILD_DIR)/ETSI_3_10.BIN
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_XRX300_US600R),y)
	PKG_ORGINAL_BASE_NAME_FXS:=$(PKG_BUILD_DIR)/R600_3_10.BIN
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_XRX300_GERMANY),y)
	PKG_ORGINAL_BASE_NAME_FXS:=$(PKG_BUILD_DIR)/GER_3_10.BIN
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_XRX300_USE_CUSTOM_FILE),y)
	PKG_ORGINAL_BASE_NAME_FXS:=$(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_VR9_CUSTOM_FILE)
endif

# FXO part
ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_XRX300_ETSI_FXO),y)
	PKG_ORGINAL_BASE_NAME_FXO:=$(PKG_BUILD_DIR)/FXO_ETSI_0_0.BIN
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_XRX300_US600R_FXO),y)
	PKG_ORGINAL_BASE_NAME_FXO:=$(PKG_BUILD_DIR)/FXO_R600_0_0.BIN
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_XRX300_GERMANY_FXO),y)
	PKG_ORGINAL_BASE_NAME_FXO:=$(PKG_BUILD_DIR)/FXO_GER_0_0.BIN
endif

ifeq ($(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_XRX300_USE_CUSTOM_FILE_FXO),y)
	PKG_ORGINAL_BASE_NAME_FXO:=$(CONFIG_LTQ_VOICE_CPE_VMMC_COEF_VR9_CUSTOM_FILE_FXO)
endif

define Package/$(PKG_NAME)/description
	XRX300 voice coefficient samples.
	Coefficients can be calculated with XTCOS.
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/$(IFX_FIRMWARE_DIR)
	$(INSTALL_DIR) $(1)/$(LANTIQ_FIRMWARE_DIR)
	# copy FXS BBD file
	$(CP) $(PKG_ORGINAL_BASE_NAME_FXS) $(1)/$(LANTIQ_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME_FXS)
	# copy FXO BBD file
	$(CP) $(PKG_ORGINAL_BASE_NAME_FXO) $(1)/$(LANTIQ_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME_FXO)
	# create links in /opt/ifx/downloads - for backward compatibility
	ln -snf $(LANTIQ_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME_FXS) $(1)/$(IFX_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME_FXS)
	ln -snf $(LANTIQ_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME_FXO) $(1)/$(IFX_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME_FXO)
	# create alias for backward compatibility
	#ln -snf $(LANTIQ_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME_FXS) $(1)/$(LANTIQ_FIRMWARE_DIR)/vr9_bbd_fxs.bin
	#ln -snf $(LANTIQ_FIRMWARE_DIR)/$(PKG_TARGET_BASE_NAME_FXO) $(1)/$(LANTIQ_FIRMWARE_DIR)/vr9_bbd_fxo.bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
