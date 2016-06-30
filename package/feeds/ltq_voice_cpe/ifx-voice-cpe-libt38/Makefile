#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: $

include $(TOPDIR)/rules.mk

PKG_NAME:=ifx-voice-cpe-libt38
PKG_VERSION:=1.29
include $(INCLUDE_DIR)/package-version-override.mk
PKG_BASE_NAME:=libt38_mips
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://localhost

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=libs
  CATEGORY:=Lantiq
  TITLE:=T.38 protocol stack library
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	This library will be used by Fax/T.38 applications.
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/t38
	$(CP) $(PKG_BUILD_DIR)/include/* $(STAGING_DIR)/usr/include/t38/
	$(INSTALL_DIR) $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/lib/* $(STAGING_DIR)/usr/lib/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
