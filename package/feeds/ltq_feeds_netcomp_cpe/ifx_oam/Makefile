#
# Openwrt Makefile for ifx-oam
#

#### Includes ###################################
include $(TOPDIR)/rules.mk

#### Package Info ###############################
PKG_NAME:=ifx-oam
PKG_VERSION:=2.3.0.12
PKG_RELEASE:=1
PKG_MD5SUM:=244d3951b4ab4d5f80753807e39e34f5
PKG_BASE_NAME:=ifx_oam
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
#PKG_FILE_DEPENDS:=$(STAGING_DIR)/usr/include/ifx_config.h
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_router/ifx_oam
PKG_NO_AUTORECONF=y

PKG_CONFIG_DEPENDS_REBUILD=y
PKG_CONFIG_DEPENDS:=\
	$(foreach _mr,$(shell grep -w config config/Config.in|awk '{ print $$2 }'),CONFIG_$(_mr))

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk
ifeq ($(CONFIG_FEATURE_IFX_HOST_ENVIR),y)
  include $(INCLUDE_DIR)/host-build.mk
endif

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	SUBMENU:=Router Subsystem
	TITLE:=Lantiq OAM Module Package for Router 4.1
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Technologies AG
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	This package contains Operations Administration & Maintenance module for router 4.x
endef

### Menuconfig ##################################
define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	menu "$(PKG_NAME) features"
	depends on PACKAGE_$(PKG_NAME)
	source "$(SOURCE)/config/Config.in"
	endmenu
endef

### Configurations depends on Menuconfig ########
IFX_CFLAGS_y:=
IFX_LDFLAGS_y:=

ifeq ($(CONFIG_FEATURE_IFX_OAM_FULL),y)
	IFX_CFLAGS_y +=
else
	IFX_CFLAGS_y +=IFX_2MB_OAM_PKG=1
endif

ifeq ($(CONFIG_FEATURE_IFX_OAM_F5_LOOPBACK),y)
	IFX_CFLAGS_y +=IFX_OAM_F5_LOOPBACK=1
endif

ifeq ($(CONFIG_FEATURE_IFX_OAM_F4_LOOPBACK),y)
	IFX_CFLAGS_y +=IFX_OAM_F4_LOOPBACK=1
endif

ifeq ($(CONFIG_FEATURE_IFX_OAM_F5_LOOPBACK_PING),y)
	IFX_CFLAGS_y +=IFX_OAM_F5_LOOPBACK_PING=1
endif

ifeq ($(CONFIG_FEATURE_IFX_OAM_F4_LOOPBACK_PING),y)
	IFX_CFLAGS_y +=IFX_OAM_F4_LOOPBACK_PING=1
endif

ifeq ($(CONFIG_FEATURE_IFX_OAM_AIS),y)
	IFX_CFLAGS_y +=IFX_OAM_AIS=1
endif

ifeq ($(CONFIG_FEATURE_IFX_OAM_RDI),y)
	IFX_CFLAGS_y +=IFX_OAM_RDI=1
endif

ifeq ($(CONFIG_FEATURE_IFX_OAM_CC),y)
	IFX_CFLAGS_y +=IFX_OAM_CC=1
endif

ifeq ($(CONFIG_FEATURE_IFX_OAM_EVENT_SCRIPT),y)
	IFX_CFLAGS_y +=IFX_OAM_EVENT_SCRIPT=1
endif

ifeq ($(CONFIG_FEATURE_IFX_OAM_SIMULATOR),y)
	IFX_CFLAGS_y +=IFX_OAM_SIMULATOR=1
endif

IFX_CFLAGS:=$(IFX_CFLAGS_y)
IFX_LDFLAGS:=$(IFX_LDFLAGS_y)

#### Export Section for Features & Others #######
ifeq ($(CONFIG_FEATURE_IFX_HOST_ENVIR),y)
  export CONFIG_FEATURE_IFX_HOST_ENVIR
endif

ifeq ($(CONFIG_FEATURE_IFX_HOST_ENVIR),y)
CONFIG_OPTS=$(HOST_CONFIGURE_VARS)
CFLAGS="-g -Wall -fno-stack-protector -Werror"
else
CONFIG_OPTS=$(TARGET_CONFIGURE_OPTS)
CFLAGS="$(TARGET_CFLAGS) -Wall -Werror"
endif

#### Target Rules ###############################
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
	$(CONFIG_OPTS) \
	$(IFX_CFLAGS) $(IFX_LDFLAGS) \
	CFLAGS=$(CFLAGS)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/oamd $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/oamctl $(1)/usr/bin/
ifeq ($(CONFIG_FEATURE_IFX_OAM_SIMULATOR),y)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/oamsim $(1)/usr/bin/
endif
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/liboamapi.so $(1)/usr/lib/
	$(INSTALL_DIR) $(STAGING_DIR)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/liboamapi.so $(STAGING_DIR)/usr/lib/
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	$(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
