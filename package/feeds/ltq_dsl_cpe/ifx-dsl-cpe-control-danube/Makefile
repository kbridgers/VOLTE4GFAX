#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-dsl-cpe-control-danube
PKG_VERSION:=4.14.16
PKG_MD5SUM:=563bde2e8599f7fd7c3ac11ea3631480
PKG_RELEASE:=1
PKG_INSTALL:=1
PKG_BASE_NAME:=dsl_cpe_control_danube
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/dsl_cpe_control-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=ifx-dsl-cpe-api-danube ifx-dsl-cpe-control-danube_DTI:lib-dti

LANTIQ_BIN_DIR=/opt/lantiq/bin
# TBR
IFX_BIN_DIR=/opt/ifx/bin

IFX_DSL_MAX_DEVICE=$(strip $(subst ",, $(CONFIG_IFX_DSL_MAX_DEVICE)))
IFX_DSL_LINES_PER_DEVICE=$(strip $(subst ",, $(CONFIG_IFX_DSL_LINES_PER_DEVICE)))
IFX_DSL_CHANNELS_PER_LINE=$(strip $(subst ",, $(CONFIG_IFX_DSL_CHANNELS_PER_LINE)))


ifeq ($(IFX_DSL_MAX_DEVICE),)
	IFX_DSL_MAX_DEVICE=1
endif

ifeq ($(IFX_DSL_LINES_PER_DEVICE),)
	IFX_DSL_LINES_PER_DEVICE=1
endif

ifeq ($(IFX_DSL_CHANNELS_PER_LINE),)
	IFX_DSL_CHANNELS_PER_LINE=1
endif

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell ( \
		grep '^CONFIG_$(PKG_NAME)' $(TOPDIR)/.config; \
		grep '^CONFIG_IFX_DSL_' $(TOPDIR)/.config) | md5s)
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
	Lantiq DSL CPE API for Danube Family and Vinax.
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	source "$(SOURCE)/Config.in"
endef

define Package/$(PKG_NAME)
	$(call Package/ifx-dsl-cpe-control)
	DEPENDS:=ifx-dsl-cpe-api-danube +libpthread
endef

define Package/$(PKG_NAME)/description
	This package contains the DSL CPE control application.

	Supported Devices:
		- AMAZON-SE Family
		- DANUBE Family
		- ARX100 Family
		- ARX300 Family
endef

CONFIGURE_ARGS += \
	--with-max-device="$(IFX_DSL_MAX_DEVICE)" \
	--with-lines-per-device="$(IFX_DSL_LINES_PER_DEVICE)" \
	--with-channels-per-line="$(IFX_DSL_CHANNELS_PER_LINE)"

CONFIGURE_ARGS += \
	--enable-danube \
	--enable-driver-include="-I$(STAGING_DIR)/usr/include/drv_dsl_cpe_api" \
	--enable-add-appl-cflags="-DMAX_CLI_PIPES=2" \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-real-time-trace=no

ifeq ($(CONFIG_ifx-dsl-cpe-control-danube_DTI),y)
CONFIGURE_ARGS += \
	--enable-dti=yes \
	--enable-dti-include="-I$(STAGING_DIR)/usr/include/dti" \
	--enable-dti-library="-L$(STAGING_DIR)/usr/lib"
endif

ifeq ($(CONFIG_IFX_CLI),y)
CONFIGURE_ARGS += \
	--enable-cli-support
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
	$(INSTALL_BIN) ./files/ltq_cpe_control_init.sh $(1)/etc/init.d/

	$(INSTALL_DIR) $(1)/$(LANTIQ_BIN_DIR)
	# TBR
	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)

	$(INSTALL_DATA) ./files/adsl.scr $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_DATA) ./files/dsl.cfg $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/dsl_cpe_control $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/alias_dsl_cpe.sh $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/dsl_cpe_pipe.sh $(1)/$(LANTIQ_BIN_DIR)
	$(SED) "s:/opt/lantiq:/opt/lantiq/bin:g" $(1)/$(LANTIQ_BIN_DIR)/alias_dsl_cpe.sh
	# TBR
	(cd $(1)/$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/dsl_cpe_control .)
	(cd $(1)/$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/dsl_cpe_pipe.sh .)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
