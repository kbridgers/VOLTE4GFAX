#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ifx-dsl-cpe-mei-vrx
PKG_VERSION:=1.3.13
PKG_MD5SUM:=40b568beb51cd59f806d26d88cd3ead1
PKG_BASE_NAME:=drv_mei_cpe
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1
PKG_INSTALL:=1
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=ifx-os
LANTIQ_BIN_DIR=opt/lantiq/bin
FW_DIR=/lib/firmware/$(LINUX_VERSION)

# Get config values from general image config or other packages in dependency list
# By default this is the reference place for definition of the following values
IFX_DSL_MAX_DEVICE=$(strip $(subst ",, $(CONFIG_IFX_VRX_MAX_DEVICE)))
IFX_DSL_LINES_PER_DEVICE=$(strip $(subst ",, $(CONFIG_IFX_VRX_LINES_PER_DEVICE)))
IFX_DSL_CHANNELS_PER_LINE=$(strip $(subst ",, $(CONFIG_IFX_VRX_CHANNELS_PER_LINE)))

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

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

ifeq ($(DUMP),)
	STAMP_CONFIGURED:=$(strip $(STAMP_CONFIGURED))_$(shell ( \
		grep '^CONFIG_$(PKG_NAME)' $(TOPDIR)/.config; \
		grep '^CONFIG_PTM_VECTORING' $(LINUX_DIR)/.config) | md5s)
endif

define Package/$(PKG_NAME)
  SECTION:=driver
  CATEGORY:=Lantiq
  TITLE:=VRx MEI CPE Driver
  URL:=http://www.Lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=+kmod-ifxos +kmod-ifx-dsl-cpe-mei-vrx
  MENU:=1
endef

define KernelPackage/$(PKG_NAME)
  SUBMENU:=Lantiq
  TITLE:=VRx MEI CPE Driver
  URL:=http://www.lantiq.com/
  MAINTAINER:=Lantiq Deutschland GmbH
  DEPENDS:=+kmod-ifxos
  KCONFIG:=CONFIG_DSL_MEI_CPE_DRV=y
endef


# package specific configuration
define Package/$(PKG_NAME)/config
$(call Package/$(PKG_NAME)/override_version)
$(call Package/$(PKG_NAME)/override_source_path)
	source "$(SOURCE)/Config.in"
endef 

define Package/$(PKG_NAME)/description
	Lantiq MEI CPE Driver
	Supported Devices:
		- VRX200 Family
		- VRX300 Family
endef

define KernelPackage/$(PKG_NAME)/description
	Lantiq MEI CPE Kernel Module Driver
endef


ifeq ($(CONFIG_$(PKG_NAME)_DEVICE_VR9),y)
CONFIGURE_ARGS += --enable-device=vr9
else ifeq ($(CONFIG_$(PKG_NAME)_DEVICE_VR10),y)
CONFIGURE_ARGS += --enable-device=vr10
else
CONFIGURE_ARGS += --enable-device=vr9
INCLUDE_DSM=-DMEI_SUPPORT_DSM=1
endif

ifeq ($(CONFIG_PTM_VECTORING),y)
INCLUDE_DSM=-DMEI_SUPPORT_DSM=1
else
INCLUDE_DSM=-DMEI_SUPPORT_DSM=0
endif

CONFIGURE_ARGS += \
	--enable-kernelincl="$(LINUX_DIR)/include" \
	--with-max-device=$(IFX_DSL_MAX_DEVICE) \
	--with-lines-per-device=$(IFX_DSL_LINES_PER_DEVICE) \
	--enable-debug --enable-error_print \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib" \
	--enable-add_drv_cflags="${INCLUDE_DSM} -DMEI_DRV_ATM_PTM_INTERFACE_ENABLE=1 -fno-pic -mno-abicalls -mlong-calls -O2 -g0"

ifeq ($(CONFIG_ifx-dsl-cpe-mei-vrx_USE_KERNEL_BUILD_IN),y)
CONFIGURE_ARGS += \
	--enable-ifxos_drv=no \
	--enable-lkm=no 
endif

ifeq ($(CONFIG_ifx-dsl-cpe-mei-vrx_USE_PCI_SLAVE_DOWNLOAD),y)
CONFIGURE_ARGS += \
	--enable-pci-slave=yes

ifeq ($(CONFIG_ifx-dsl-cpe-mei-vrx_USE_PCI_SLAVE_ADDR_RANGE_ADTRAN),y)
CONFIGURE_ARGS += \
	--enable-pci-slave-addr=adtran
else ifeq ($(CONFIG_ifx-dsl-cpe-mei-vrx_USE_PCI_SLAVE_ADDR_RANGE_BONDING),y)
CONFIGURE_ARGS += \
	--enable-pci-slave-addr=bonding
else
CONFIGURE_ARGS += \
	--enable-pci-slave-addr=bonding
endif
endif

ifeq ($(CONFIG_LINUX_2_6),y)
CONFIGURE_ARGS += \
	--enable-linux-26 \
	--enable-kernelbuild="$(LINUX_DIR)" \
	ARCH=$(LINUX_KARCH)
PKG_MODULE_EXTENSION=.ko
endif

define Build/Clean
	@- [ -e $(PKG_BUILD_DIR)/ipkg ] && rm -Rf $(PKG_BUILD_DIR)/ipkg
	@- [ -e $(PKG_BUILD_DIR)/Makefile ] && $(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Clean/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/{lib,include}
	$(CP) $(PKG_BUILD_DIR)/src/drv_mei_cpe_api_intern.h $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/src/drv_mei_cpe_api_atm_ptm_intern.h $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/src/drv_mei_cpe_interface.h $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/src/drv_mei_cpe_config.h $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/src/cmv_message_format.h $(1)/usr/include/
endef

define Build/UninstallDev
	rm -rf $(BUILD_DIR)/$(PKG_BASE_NAME)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/ltq_load_cpe_mei_drv.sh $(1)/etc/init.d/
	# Set number of entities within startup script
	entities=`expr $(IFX_DSL_LINES_PER_DEVICE) \* $(IFX_DSL_MAX_DEVICE)`; \
		$(SED) "s:@entities@:$$$$entities:g" $(1)/etc/init.d/ltq_load_cpe_mei_drv.sh; \
		
	$(INSTALL_DIR) $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/alias_drv_mei_cpe.sh $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/inst_drv_mei_cpe.sh $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scripts/enable_ARC_JTAG.sh $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/mei_cpe_drv_test $(1)/$(LANTIQ_BIN_DIR)

ifeq ($(CONFIG_ifx-dsl-cpe-mei-vrx_USE_KERNEL_BUILD_IN),y)
	$(INSTALL_DIR) $(1)/$(FW_DIR)
	$(CP) $(PKG_BUILD_DIR)/src/$(PKG_BASE_NAME).o $(LINUX_DIR)/drivers/char/
else
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/$(PKG_BASE_NAME)$(PKG_MODULE_EXTENSION) $(1)/$(LANTIQ_BIN_DIR)
	$(INSTALL_DIR) $(1)/$(MODULES_SUBDIR)
	ln -snf /$(LANTIQ_BIN_DIR)/$(PKG_BASE_NAME)$(PKG_MODULE_EXTENSION) $(1)/$(MODULES_SUBDIR)/$(PKG_BASE_NAME).$(LINUX_KMOD_SUFFIX)

	$(INSTALL_DIR) $(1)/$(FW_DIR)
endif
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call KernelPackage,$(PKG_NAME)))
