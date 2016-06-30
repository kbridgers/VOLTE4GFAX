# 
# Openwrt Makefile for ifx-config_common_features
#

#### Includes ###################################
include $(TOPDIR)/rules.mk

#### Package Info ###############################
PKG_NAME:=ifx-config_common_features
PKG_VERSION:=1.1.1
PKG_RELEASE:=1
PKG_BASE_NAME:=ifx_config_common_features
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-${PKG_VERSION}
PKG_SOURCE_URL:=http://localhost
#PKG_FILE_DEPENDS:=$(STAGING_DIR)/usr/include/ifx_config.h
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_router/ifx_config_common_features
PKG_NO_AUTORECONF=y

PKG_CONFIG_DEPENDS_REBUILD=y
PKG_CONFIG_DEPENDS:=\
        $(foreach _mr,$(shell grep -w config config/Config.in|awk '{ print $$2 }'),CONFIG_$(_mr)) 

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	SUBMENU:=Router Subsystem
	TITLE:=Lantiq Common Configuration Features module (Non Functional)
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Technologies AG
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	This package contain Lantiq Common Configuration Features (CONFIG_FEATURE) options
endef

### Menuconfig ##################################
define Package/$(PKG_NAME)/config
	menu "$(PKG_NAME) features"
		depends on PACKAGE_$(PKG_NAME)
		source "$(SOURCE)/config/Config.in"
	endmenu
	config PACKAGE_$(PKG_NAME)
		default y
endef

### Define feature wise rootfs scripts to install in 'INSTALL_FILES_y' macro
INSTALL_FILES_y:=
INSTALL_FILES_$(if $(CONFIG_LTQ_BRIDGE_MODEM),,y) += ./files/*
INSTALL_FILES_$(CONFIG_FEATURE_CELL_WAN_SUPPORT) += ./files-opt/wwan/*
INSTALL_FILES_$(CONFIG_FEATURE_DUAL_WAN_SUPPORT) += ./files-opt/dual_wan/*
ifeq ($(shell [[ "A$(CONFIG_FEATURE_ADSL_WAN_SUPPORT)" == "Ay" || "A$(CONFIG_FEATURE_PTM_WAN_SUPPORT)" == "Ay" ]] && echo true ),true)
	INSTALL_FILES_y += ./files-opt/dsl/*
endif
ifeq ($(shell [[ "A$(CONFIG_FEATURE_ADSL_WAN_SUPPORT)" == "Ay" || "A$(CONFIG_FEATURE_PTM_WAN_SUPPORT)" == "Ay" || \
"A$(CONFIG_FEATURE_ETH_WAN_SUPPORT)" == "Ay" ]] && echo true ),true)
#ifeq ($(CONFIG_FEATURE_ADSL_WAN_SUPPORT),y) || ($(CONFIG_FEATURE_PTM_WAN_SUPPORT),y) || ($(CONFIG_FEATURE_ETH_WAN_SUPPORT),y)
	INSTALL_FILES_y += ./files-opt/wan/common/*
endif
INSTALL_FILES_$(CONFIG_FEATURE_ANY_WAN_SUPPORT) += ./files-opt/wan/anywan_demo/*
INSTALL_FILES_y += ./files-opt/route/common/*
INSTALL_FILES_$(CONFIG_FEATURE_RIP) += ./files-opt/route/rip/*
INSTALL_FILES_$(POLICY_ROUTING) += ./files-opt/route/policy/*
INSTALL_FILES_$(CONFIG_FEATURE_DHCP_SERVER) += ./files-opt/lan/dhcp/*
INSTALL_FILES_$(CONFIG_PACKAGE_lq-igmpd) += ./files-opt/igmp/*
INSTALL_FILES_$(CONFIG_FEATURE_LTQ_PORT_WAN_BINDING) +=./files-opt/route/pwb/*
INSTALL_FILES_y += ./files-opt/firewall/*
INSTALL_FILES_y += ./files-opt/wan/services/*
INSTALL_FILES_y += ./files-opt/nat/*
INSTALL_FILES_y += ./files-opt/ipv6/*
INSTALL_FILES_y += ./files-opt/firewall6/*
INSTALL_FILES_y += ./files-opt/qos/*

#### Target Rules ###############################
define Build/Compile
	echo -n ""
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/ifx_httpd/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_httpd/*.h $(STAGING_DIR)/usr/include/ifx_httpd/.
	$(foreach i_file, $(INSTALL_FILES_y), \
		$(CP) $(i_file) $(1)/.; \
	)
	$(if $(CONFIG_LTQ_BRIDGE_MODEM), $(if $(CONFIG_FEATURE_LTQ_WIRELESS_VB), \
		$(INSTALL_DIR) $(1)/etc/rc.d; \
		$(CP) -f ./files-opt/ipv6/etc/rc.d/start_ipv6_apps $(1)/etc/rc.d/
	))
endef
#####

$(eval $(call BuildPackage,$(PKG_NAME)))
