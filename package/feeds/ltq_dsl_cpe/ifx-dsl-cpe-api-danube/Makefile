#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-dsl-cpe-api-danube
PKG_VERSION:=4.14.16
PKG_MD5SUM:=8895e745ade2818d2c9e35a3bb781e6d
PKG_RELEASE:=1
PKG_INSTALL:=1
PKG_BASE_NAME:=drv_dsl_cpe_api_danube
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/drv_dsl_cpe_api-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=ifx-dsl-cpe-mei-danube ifxos

LANTIQ_BIN_DIR=/opt/lantiq/bin
# TBR
IFX_BIN_DIR=/opt/ifx/bin

# get config values from general image config or other packages in dependency list
IFX_DSL_MAX_DEVICE=$(strip $(subst ",, $(CONFIG_IFX_DSL_MAX_DEVICE)))
IFX_DSL_LINES_PER_DEVICE=$(strip $(subst ",, $(CONFIG_IFX_DSL_LINES_PER_DEVICE)))
IFX_DSL_CHANNELS_PER_LINE=$(strip $(subst ",, $(CONFIG_IFX_DSL_CHANNELS_PER_LINE)))
IFX_DSL_DRIVER_INCLUDE=$(strip $(subst ",, $(CONFIG_IFX_DSL_DRIVER_INCLUDE)))

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

define Build/Compile
# add installation
	$(call Build/Compile/Default, DESTDIR="$(PKG_INSTALL_DIR)" CC="$(TARGET_CC)" all install )
endef

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=DSL CPE API driver
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=+kmod-ifxos
  MENU:=1
endef

define Package/$(PKG_NAME)/description
	This package contains the Lantiq DSL CPE API driver.

	Supported Devices:
		- DANUBE Family
		- AMAZON-SE Family
		- ARX100 Family.
endef

define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	source "$(SOURCE)/Config.in"
endef

CONFIGURE_ARGS += --enable-kernel-include="$(LINUX_DIR)/include" \
	--with-max-device="$(IFX_DSL_MAX_DEVICE)" \
	--with-lines-per-device="$(IFX_DSL_LINES_PER_DEVICE)" \
	--with-channels-per-line="$(IFX_DSL_CHANNELS_PER_LINE)"

CONFIGURE_ARGS += \
	--enable-danube \
	--enable-add-drv-cflags="-DMODULE" \
	--enable-ifxos=yes \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-firmware-memory-free=no \
	--enable-real-time-trace=no

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

ifeq ($(CONFIG_LINUX_2_6),y)
CONFIGURE_ARGS += --enable-linux-26 --enable-kernelbuild="$(LINUX_DIR)"
PKG_MODULE_EXTENSION=.ko
endif

ifeq ($(CONFIG_mips),y)
EXTRA_CFLAGS = -fno-pic -mno-abicalls -mlong-calls -G 0
endif

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/ipkg ] && rm -Rf $(PKG_BUILD_DIR)/ipkg
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/drv_dsl_cpe_api
	$(CP) $(PKG_BUILD_DIR)/src/include/drv_dsl_cpe*.h $(1)/usr/include/drv_dsl_cpe_api/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/ltq_load_dsl_cpe_api.sh $(1)/etc/init.d/

	$(INSTALL_DIR) $(1)/$(LANTIQ_BIN_DIR)
	# TBR
	$(INSTALL_DIR) $(1)/$(IFX_BIN_DIR)

	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/inst_drv_dsl_cpe_api_danube.sh $(1)/$(LANTIQ_BIN_DIR)/inst_drv_cpe_api.sh
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/drv_dsl_cpe_api$(PKG_MODULE_EXTENSION) $(1)/$(LANTIQ_BIN_DIR)
	# TBR
	(cd $(1)/$(IFX_BIN_DIR); ln -snf ../../lantiq/bin/drv_dsl_cpe_api$(PKG_MODULE_EXTENSION) .)

endef

$(eval $(call BuildPackage,$(PKG_NAME)))
