#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-dsl-cpe-control-vrx
PKG_VERSION:=4.14.16
PKG_MD5SUM:=a3629115b463ab9fc9c8543c4c8351a5
PKG_RELEASE:=1
PKG_BASE_NAME:=dsl_cpe_control_vrx
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/dsl_cpe_control-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=ifx-dsl-cpe-api-vrx ifx-dsl-cpe-mei-vrx ifx-dsl-cpe-control-vrx_DTI:lib-dti

LANTIQ_BIN_DIR=/opt/lantiq/bin
# TBR
IFX_BIN_DIR=/opt/ifx/bin

# Get config values from general image config or other packages in dependency list
# Definition of these values will be overtaken from package 'ifx-dsl-cpe-mei-vrx'
IFX_DSL_MAX_DEVICE=$(strip $(subst ",, $(CONFIG_IFX_VRX_MAX_DEVICE)))
IFX_DSL_LINES_PER_DEVICE=$(strip $(subst ",, $(CONFIG_IFX_VRX_LINES_PER_DEVICE)))
IFX_DSL_CHANNELS_PER_LINE=$(strip $(subst ",, $(CONFIG_IFX_VRX_CHANNELS_PER_LINE)))

IFX_DSL_TOTAL_ENTITIES:=$(shell echo $$[$(IFX_DSL_MAX_DEVICE)*$(IFX_DSL_LINES_PER_DEVICE)])

# Set default config values if necessary (not defined before)
ifeq ($(IFX_DSL_MAX_DEVICE),)
	IFX_DSL_MAX_DEVICE=1
endif

ifeq ($(IFX_DSL_LINES_PER_DEVICE),)
	IFX_DSL_LINES_PER_DEVICE=1
endif

ifeq ($(IFX_DSL_CHANNELS_PER_LINE),)
	IFX_DSL_CHANNELS_PER_LINE=1
endif

ifeq ($(CONFIG_IFX_CLI),)
CONFIG_IFX_CLI=y
endif

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell ( \
		grep '^CONFIG_$(PKG_NAME)' $(TOPDIR)/.config; \
		grep '^CONFIG_IFX_DSL_' $(TOPDIR)/.config; \
		grep '^CONFIG_IFX_VRX_' $(TOPDIR)/.config; \
		grep '^CONFIG_PTM_VECTORING' $(LINUX_DIR)/.config) | md5s)
endif

define Package/ifx-dsl-cpe-control
	SECTION:=application
	CATEGORY:=Lantiq
	TITLE:=DSL CPE control application
	URL:=http://www.lantiq.com/
	MAINTAINER:=Lantiq Deutschland GmbH
	PROVIDES:=ifx-dsl-cpe-control
	MENU:=1
endef

define Package/ifx-dsl-cpe-control/description
	Lantiq DSL CPE for Amazon SE, Danube,Vinax and VRx.
endef

define Package/$(PKG_NAME)
	$(call Package/ifx-dsl-cpe-control)
	DEPENDS:=ifx-dsl-cpe-api-vrx +libpthread
endef

define Package/$(PKG_NAME)/description
	This package contains the DSL CPE control application.

	Supported Devices:
		- VRX200 Family
		- VRX300 Family
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	source "$(SOURCE)/Config.in"
endef

CONFIGURE_ARGS += \
	--with-max-device="$(IFX_DSL_MAX_DEVICE)" \
	--with-lines-per-device="$(IFX_DSL_LINES_PER_DEVICE)" \
	--with-channels-per-line="$(IFX_DSL_CHANNELS_PER_LINE)"

CONFIGURE_ARGS += \
	--enable-vrx \
	--enable-driver-include="-I$(STAGING_DIR)/usr/include/drv_dsl_cpe_api" \
	--enable-device-driver-include="-I$(STAGING_DIR)/usr/include" \
	--enable-add-appl-cflags="-DMAX_CLI_PIPES=2" \
	--enable-ifxos \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-ifxos-library="-I$(STAGING_DIR)/usr/lib" \
	--enable-dsl-ceoc=no

ifeq ($(CONFIG_ifx-dsl-cpe-control-vrx_DTI),y)
CONFIGURE_ARGS += \
	--enable-dti=yes \
	--enable-dti-include="-I$(STAGING_DIR)/usr/include/dti" \
	--enable-dti-library="-L$(STAGING_DIR)/usr/lib"
endif

ifeq ($(CONFIG_IFX_CLI),y)
CONFIGURE_ARGS += \
	--enable-cli-support
endif

ifeq ($(CONFIG_$(PKG_NAME)_DEVICE_VR9),y)
CONFIGURE_ARGS += --enable-vrx-device=vr9
else ifeq ($(CONFIG_$(PKG_NAME)_DEVICE_VR10),y)
CONFIGURE_ARGS += --enable-vrx-device=vr10
else
CONFIGURE_ARGS += --enable-vrx-device=vr9
endif

ifeq ($(CONFIG_$(PKG_NAME)_MODEL_LITE),y)
CONFIGURE_ARGS += --enable-model=lite
else ifeq ($(CONFIG_$(PKG_NAME)_MODEL_FOOTPRINT),y)
CONFIGURE_ARGS += --enable-model=footprint
else ifeq ($(CONFIG_$(PKG_NAME)_MODEL_TYPICAL),y)
CONFIGURE_ARGS += --enable-model=typical
else ifeq ($(CONFIG_$(PKG_NAME)_MODEL_FULL),y)
CONFIGURE_ARGS += --enable-model=full
else ifeq ($(CONFIG_$(PKG_NAME)_MODEL_DEBUG),y)
CONFIGURE_ARGS += --enable-model=debug
else
CONFIGURE_ARGS += --enable-model=footprint
endif

ifeq ($(CONFIG_$(PKG_NAME)_USE_ADDITIONAL_BUILD_OPTIONS),y)
CONFIGURE_ARGS += $(CONFIG_$(PKG_NAME)_ADDITIONAL_BUILD_OPTIONS)
endif

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/ipkg ] && rm -Rf $(PKG_BUILD_DIR)/ipkg
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/ltq_dsl_fw_dbg.sh $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/ltq_cpe_control_init.sh $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/$(LANTIQ_BIN_DIR)
	# TBR
	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)

ifeq ($(IFX_DSL_LINES_PER_DEVICE),1)
	$(INSTALL_DATA) ./files/adsl.scr $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_DATA) ./files/vdsl.scr $(1)/$(LANTIQ_BIN_DIR)
else
	$(INSTALL_DATA) ./files/adsl_bonding.scr $(1)/$(LANTIQ_BIN_DIR)/adsl.scr
	$(INSTALL_DATA) ./files/vdsl_bonding.scr $(1)/$(LANTIQ_BIN_DIR)/vdsl.scr
endif

	# Set support flag for L2 vectoring
ifeq ($(CONFIG_PTM_VECTORING),y)
	$(SED) "s:@vectoring_l2_cfg@:1:g" $(1)/etc/init.d/ltq_cpe_control_init.sh;
else
	$(SED) "s:@vectoring_l2_cfg@:0:g" $(1)/etc/init.d/ltq_cpe_control_init.sh;
endif

	# Set support flag for bonding
	echo IFX_DSL_TOTAL_ENTITIES = $(IFX_DSL_TOTAL_ENTITIES)
ifeq ($(IFX_DSL_TOTAL_ENTITIES),1)
	$(SED) "s:@bonding_cfg@:0:g" $(1)/etc/init.d/ltq_cpe_control_init.sh;
else
	$(SED) "s:@bonding_cfg@:1:g" $(1)/etc/init.d/ltq_cpe_control_init.sh;
endif

	@- [ -e ./files/dsl.cfg ] && $(INSTALL_DATA) ./files/dsl.cfg $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/dsl_cpe_control $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/alias_dsl_cpe.sh $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/dsl_cpe_pipe.sh $(1)/$(LANTIQ_BIN_DIR)
	$(SED) "s:/opt/lantiq:/opt/lantiq/bin:g" $(1)/$(LANTIQ_BIN_DIR)/alias_dsl_cpe.sh
	# TBR
	(cd $(1)/$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/dsl_cpe_control .)
	(cd $(1)/$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/dsl_cpe_pipe.sh .)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
