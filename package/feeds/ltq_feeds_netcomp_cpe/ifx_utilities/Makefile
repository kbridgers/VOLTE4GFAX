#
# Openwrt Makefile for ifx-utilities
#

#### Includes ###################################
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

#### Package Info ###############################
PKG_NAME:=ifx-utilities
PKG_VERSION:=2.3.0.70
PKG_RELEASE:=1
PKG_MD5SUM:=2859c72a7492f67c5be9b782add1a6e9
PKG_BASE_NAME:=ifx_utilities
PKG_SOURCE:=$(PKG_BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://localhost
PKG_BUILD_DEPENDS:=PACKAGE_ifx-dsl-cpe-api-danube:ifx-dsl-cpe-api-danube PACKAGE_ifx-dsl-cpe-control-danube:ifx-dsl-cpe-control-danube PACKAGE_ifx-dsl-cpe-api-vrx:ifx-dsl-cpe-api-vrx PACKAGE_ifx-dsl-cpe-api-vrx-bonding:ifx-dsl-cpe-api-vrx-bonding PACKAGE_ifx-dsl-cpe-control-vrx:ifx-dsl-cpe-control-vrx PACKAGE_ifx-dsl-cpe-control-vrx-bonding:ifx-dsl-cpe-control-vrx-bonding ifx-IFXAPIs ifx-httpd
#PKG_FILE_DEPENDS:=$(STAGING_DIR)/usr/include/ifx_config.h
PKG_DEFAULT_CUSTOM_SOURCE_DIR:=$(TOPDIR)/../../../wlnsw/wireline_sw_router/ifx_utilities
PKG_NO_AUTORECONF=y

PKG_CONFIG_DEPENDS_REBUILD=y
PKG_CONFIG_DEPENDS:=\
        $(foreach _mr,$(shell grep -w config config/Config.in|awk '{ print $$2 }'),CONFIG_$(_mr)) \
        $(shell cat config/configs_used.list)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-version-override.mk

define Package/$(PKG_NAME)
	SECTION:=utils
	CATEGORY:=Lantiq
	SUBMENU:=Router Subsystem
	TITLE:=Lantiq package utilities for modules
	URL:=http://www.lantiq.com
	MAINTAINER:=Lantiq Technologies AG
	DEPENDS:=+mtd-utils
	MENU:=1
endef

define Package/$(PKG_NAME)/description
	This package contains Lantiq package utilities for IFX modules.
endef

#### Directory Paths ############################
IFX_APIS_DIR:=$(wildcard $(BUILD_DIR)/ifx_IFXAPIs*)
IFX_HTTPD_DIR:=$(wildcard $(BUILD_DIR)/ifx_httpd*)
IFX_DSL_CPE_API_INCLUDES:=$(STAGING_DIR)/usr/include/drv_dsl_cpe_api/
UBOOT_DIR:=$(wildcard $(KERNEL_BUILD_DIR)/u-boot-*)
MTD_DIR:=$(wildcard $(BUILD_DIR)/mtd-utils*)
MTD_UTIL_DIR:=$(MTD_DIR)/
export IFX_SWITCH_UTIL_DIR:=$(wildcard $(BUILD_DIR)/ifx-ethsw-utilities*)

AUTOGENCONF_DIR:=$(PKG_BUILD_DIR)/AutoGenConf
PLATFORM_CONFIG_FILE:=$(PLATFORM_DIR)/base-files/etc/rc.conf
TARGET=$(TARGET_CROSS)

# Temperory fix in 5.1 to compile utilities without uboot functions.
# To be removed when compatible to new u-boot.

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

IFX_CFLAGS_y +=$(TARGET_CFLAGS) -Wall  -DBUILD_FROM_IFX_UTILITIES -DBUILD_FROM_LTQ_APPS

IFX_CFLAGS_y +=-I$(IFX_APIS_DIR)/include
IFX_CFLAGS_y +=-I$(STAGING_DIR)/usr/include

ifeq ($(CONFIG_UBOOT_CONFIG_BOOT_FROM_NOR),y)
IFX_CFLAGS_y +=-DCONFIG_BOOT_FROM_NOR
endif

ifeq ($(CONFIG_UBOOT_CONFIG_BOOT_FROM_SPI),y)
IFX_CFLAGS_y +=-DCONFIG_BOOT_FROM_SPI
endif

ifeq ($(CONFIG_UBOOT_CONFIG_BOOT_FROM_NAND),y)
IFX_CFLAGS_y +=-DCONFIG_BOOT_FROM_NAND
IFX_CFLAGS_y +=-DCONFIG_UBOOT_CONFIG_ENV_SIZE=$(CONFIG_UBOOT_CONFIG_ENV_SIZE)
endif

ifeq ($(CONFIG_PACKAGE_ifx-IFXAPIs),y)
	IFX_MULTILIB_UTIL=y
endif

ifeq ($(CONFIG_PACKAGE_ifx-devm),y)
	IFX_CFLAGS_y +=-DTR69_DEFINED -DCONFIG_PACKAGE_IFX_DEVM
endif


ifeq ($(CONFIG_TARGET_$(BOARD)_platform_danube),y)
        IFX_CFLAGS_y +=-DPLATFORM_DANUBE
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar9),y)
        IFX_CFLAGS_y +=-DPLATFORM_AR9
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_vbg400),y)
	IFX_CFLAGS_y +=-DPLATFORM_VBG400
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar9_VB),y)
                IFX_CFLAGS_y +=-DPLATFORM_VB300
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_amazon_se),y)
        IFX_CFLAGS_y +=-DPLATFORM_AMAZON_SE
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_vr9),y)
        IFX_CFLAGS_y +=-DPLATFORM_VR9
endif
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10),y)
ifeq ($(CONFIG_TARGET_$(BOARD)_platform_ar10_vrx318),y)
        IFX_CFLAGS_y +=-DPLATFORM_AR10_VRX318 -DPLATFORM_VR9
else
        IFX_CFLAGS_y +=-DPLATFORM_AR9 -DPLATFORM_AR10
endif
endif
IFX_CFLAGS_$(CONFIG_TARGET_$(BOARD)_platform_hn1) += -DPLATFORM_HN1

ifeq ($(CONFIG_PACKAGE_ifx-dsl-cpe-api-danube),y)
	IFX_CFLAGS_y +=-DLINUX \
		-I$(IFX_DSL_CPE_API_INCLUDES) \
		-DDSL_CHANNELS_PER_LINE=1
	
	ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON_SE")
		IFX_CFLAGS_y  +=-DINCLUDE_DSL_CPE_API_DANUBE -DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif

	ifeq ($(CONFIG_IFX_CONFIG_CPU),"DANUBE")
		IFX_CFLAGS_y  +=-DINCLUDE_DSL_CPE_API_DANUBE -DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif

	ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON_S")
		IFX_CFLAGS_y  +=-DINCLUDE_DSL_CPE_API_DANUBE -DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif
	ifeq ($(CONFIG_IFX_CONFIG_CPU),"XRX3XX")
		IFX_CFLAGS_y  +=-DINCLUDE_DSL_CPE_API_DANUBE -DCONFIG_PACKAGE_IFX_DSL_CPE_API
	endif
else
  ifeq ($(CONFIG_PACKAGE_ifx-dsl-cpe-api-vrx),y)
   	IFX_CFLAGS_y +=-DLINUX \
		-I$(IFX_DSL_CPE_API_INCLUDES) \
		-DDSL_CHANNELS_PER_LINE=1

	IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_VRX -DCONFIG_PACKAGE_IFX_DSL_CPE_API
        export CONFIG_PACKAGE_IFX_DSL_CPE_API=y
  endif
  ifeq ($(CONFIG_PACKAGE_ifx-dsl-cpe-api-vrx-bonding),y)
   	IFX_CFLAGS_y +=-DLINUX \
		-I$(IFX_DSL_CPE_API_INCLUDES) \
		-DDSL_CHANNELS_PER_LINE=1

	IFX_CFLAGS_y +=-DINCLUDE_DSL_CPE_API_VRX -DCONFIG_PACKAGE_IFX_DSL_CPE_API
        export CONFIG_PACKAGE_IFX_DSL_CPE_API=y
  endif

endif

IFX_CFLAGS_y += -DCONFIG_IFX_CONFIG_MEMORY_SIZE=$(CONFIG_IFX_CONFIG_MEMORY_SIZE) \
		-DCONFIG_IFX_CONFIG_FLASH_SIZE=$(CONFIG_IFX_CONFIG_FLASH_SIZE) \
		-DCONFIG_GCC_VERSION=\\\"$(CONFIG_GCC_VERSION)\\\" \
		-DCONFIG_LIBC_VERSION=\\\"$(CONFIG_LIBC_VERSION)\\\" \
		-DCPU_$(CONFIG_IFX_CONFIG_CPU) -DIFX_BOARD_SWRESET_GPIO=$(IFX_BOARD_SWRESET_GPIO) \
		-I$(UBOOT_DIR)/include/configs

ifeq ($(CONFIG_IFX_CONFIG_FLASH_TYPE),"SPI")
	IFX_CFLAGS_y     +=-DCFG_BOOT_FROM_SPI -DCONFIG_SPI_FLASH_$(CONFIG_IFX_CONFIG_FLASH_SIZE)M
else
	ifeq ($(CONFIG_IFX_CONFIG_FLASH_TYPE),"NOR")
		IFX_CFLAGS_y     +=-DCFG_BOOT_FROM_NOR -DCONFIG_NOR_FLASH_$(CONFIG_IFX_CONFIG_FLASH_SIZE)M
	else 
		ifeq ($(CONFIG_IFX_CONFIG_FLASH_TYPE),"NAND")
			IFX_CFLAGS_y     +=-DCFG_BOOT_FROM_NAND	-DCONFIG_NAND_FLASH_$(CONFIG_IFX_CONFIG_FLASH_SIZE)M
		endif
	endif
endif

ifeq ($(MODEL_BOARD),"AXRX100_UTA")
    PLATFORM_CFLAGS         +=-DBOARD_XRX100_UTA
endif

#ifeq ($(CONFIG_UBOOT_CONFIG_FIRMWARE_IN_ROOTFS),y)
	IFX_CFLAGS_$(CONFIG_UBOOT_CONFIG_FIRMWARE_IN_ROOTFS) +=-DCONFIG_UBOOT_CONFIG_FIRMWARE_IN_ROOTFS=1 -DCONFIG_FIRMWARE_IN_ROOTFS
#else
#	IFX_CFLAGS_y     +=-DCONFIG_FEATURE_IFX_ADSL_FIRMWARE_IN_ROOTFS=0
#endif

ifeq ($(CONFIG_UBOOT_CONFIG_OVERLAY),y)
	IFX_CFLAGS_y+=-DCONFIG_UBOOT_CONFIG_OVERLAY
endif
	#IFX_CFLAGS_y += -DIFX_DEBUG

ifeq ($(CONFIG_PACKAGE_ifx-httpd),y)
	IFX_CFLAGS_y += -I$(IFX_HTTPD_DIR)
endif

IFX_CFLAGS_$(CONFIG_FEATURE_LQ_NEW_UPGRADE) += -DCONFIG_FEATURE_LQ_NEW_UPGRADE

IFX_CFLAGS_y += -I$(UBOOT_DIR)/include \
		-I$(LINUX_DIR)/include \
		-DBUILD_FOR_KERNEL -DIFX_IMAGE_EXTRA_CHECKS \
		-I$(MTD_DIR)/include -I$(MTD_UTIL_DIR) -I$(MTD_DIR)

IFX_CFLAGS_$(CONFIG_FEATURE_IFX_WIRELESS_RALINK) += -DCONFIG_FEATURE_IFX_WIRELESS_RALINK

ifeq ($(CONFIG_FEATURE_IFX_VOIP),y)
	ifeq ($(CONFIG_IFX_CONFIG_CPU),"DANUBE")
		IFX_CFLAGS_y += -I../tapi/drv_tapi/src/common/src -I../tapi/drv_tapi/src/
	else ifeq ($(CONFIG_IFX_CONFIG_CPU),"AMAZON")
		IFX_CFLAGS_y += -I../tapi/drv_vinetic/src/common/src/ -I../tapi/drv_vinetic/src/tapi/src/
	endif
endif

IFX_CFLAGS_$(CONFIG_TARGET_UBI_MTD_SUPPORT) += -DCONFIG_TARGET_UBI_MTD_SUPPORT
IFX_CFLAGS_$(CONFIG_UBOOT_CONFIG_DUAL_IMAGE) += -DCONFIG_UBOOT_CONFIG_DUAL_IMAGE
IFX_CFLAGS_$(CONFIG_UBOOT_CONFIG_ENV_REDUND) += -DCONFIG_UBOOT_CONFIG_ENV_REDUND
IFX_CFLAGS_$(CONFIG_UBOOT_CONFIG_VR9_GPHY_FW_EMBEDDED) += -DGPHY_FW_EMBEDDED -DCONFIG_VR9_GPHY_FW_EMBEDDED
IFX_CFLAGS_$(CONFIG_UBOOT_CONFIG_HN1_GPHY_FW_EMBEDDED) += -DGPHY_FW_EMBEDDED

IFX_CFLAGS_y +=-I$(INCLUDE_DIR)

IFX_LDFLAGS_y +=-L$(IFX_APIS_DIR)/lib
IFX_LDFLAGS_y +=-L$(STAGING_DIR)/usr/lib -Wl,-rpath-link,$(STAGING_DIR)/usr/lib

IFX_CFLAGS:=$(IFX_CFLAGS_y)
IFX_LDFLAGS:=$(IFX_LDFLAGS_y)

COMPILE_OPTIONS:=

ifeq ($(IFX_MULTILIB_UTIL),y)
	COMPILE_OPTIONS +=mknod_util status_oper next_macaddr \
		free_dma upgrade swreset ppasessmgmtd syscfg_lock ifx_event_util
else
	COMPILE_OPTIONS +=all
endif

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)/pagebutton"

#### Export Section for Features & Others #######
export CONFIG_IFX_MODEL_NAME TARGET CONFIG_FEATURE_IFX_TR69_DEVICE CONFIG_UBOOT_CONFIG_BOOT_FROM_NAND
export MTD_DIR MTD_UTIL_DIR CONFIG_PACKAGE_IFX_DEVM CONFIG_PACKAGE_ifx-devm CONFIG_PACKAGE_IFX_DSL_CPE_API
export IFX_CONFIG_CUSTOM_BOARD_SUPPORT CONFIG_FEATURE_IFX_VOIP CONFIG_FEATURE_NAPT CONFIG_FEATURE_IFX_DEBUG
export CONFIG_FEATURE_FIREWALL CONFIG_FEATURE_IFX_ADM6996_UTILITY CONFIG_FEATURE_LTQ_PARAMETER_ENCRYPTION
export CONFIG_GCC_VERSION CONFIG_LIBC_VERSION
#CONFIG_FEATURE_IFX_PMCU_DEMO
export CONFIG_FEATURE_IFX_VOIP IFX_CONFIG_CUSTOM_BOARD_SUPPORT
export CONFIG_PACKAGE_ifx-IFXAPIs CONFIG_PACKAGE_ifx-voip CONFIG_FEATURE_IFX_IPQOS CONFIG_FEATURE_LQ_PPA_SESSION_MANAGEMENT
export KERNEL_INCLUDE_PATH=$(LINUX_DIR)/include
export KERNEL=$(LINUX_DIR)
#Export Section for rc.conf optimisation 
ifeq ($(CONFIG_RC_CONF_RW_OPTIMISATION),y)
export CONFIG_RC_CONF_RW_OPTIMISATION
endif


#### Target Rules ###############################
define Build/Prepare
$(call Build/Prepare/Default)
	ln -fs $(UBOOT_DIR)/common/cmd_upgrade.c $(PKG_BUILD_DIR)/upgrade_cmd.c
	ln -fs $(UBOOT_DIR)/common/upgrade.c $(PKG_BUILD_DIR)/upgrade_core.c
	ln -fs $(UBOOT_DIR)/include/configs/ifx_cfg.h $(PKG_BUILD_DIR)/uboot_cfg.h
	#ln -fs $(UBOOT_DIR)/common/cmd_upgrade.o $(PKG_BUILD_DIR)/upgrade_cmd.o
	ln -fs $(IFX_APIS_DIR)/src/mapi/crypt.c	 $(PKG_BUILD_DIR)/crypt.c
	ln -fs $(IFX_APIS_DIR)/src/mapi/crypt.c	 $(AUTOGENCONF_DIR)/crypt.c
	ln -fs $(PKG_BUILD_DIR)/ltq_crypt.c  $(AUTOGENCONF_DIR)/ltq_crypt.c
	ln -fs $(PKG_BUILD_DIR)/rcCnf_crypt.c  $(AUTOGENCONF_DIR)/rcCnf_crypt.c
	ln -fs $(PKG_BUILD_DIR)/rcCnf_crypt.h  $(AUTOGENCONF_DIR)/rcCnf_crypt.h
endef

define Build/Compile
	$(MAKE) CC=gcc PLATFORM_CONFIG_FILE="$(PLATFORM_CONFIG_FILE)" -C $(AUTOGENCONF_DIR)
	$(if $(IFX_MULTILIB_UTIL),$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) \
		IFX_CFLAGS="$(IFX_CFLAGS) -DIFX_MULTILIB_UTIL" IFX_LDFLAGS="$(IFX_LDFLAGS)" ifx_util)
	$(MAKE) -C $(PKG_BUILD_DIR) \
	$(TARGET_CONFIGURE_OPTS) \
	IFX_CFLAGS="$(IFX_CFLAGS)" IFX_LDFLAGS="$(IFX_LDFLAGS)" \
	MTD_DIR="$(MTD_DIR)" \
	$(COMPILE_OPTIONS)

	$(if $(CONFIG_TARGET_ltqcpe_platform_vbg400),,$(MAKE) ARCH=$(LINUX_KARCH) $(TARGET_CONFIGURE_OPTS) -C $(LINUX_DIR) M=$(PKG_BUILD_DIR)/resetbutton modules)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(AUTOGENCONF_DIR)/rc.conf.gz $(1)/etc/
	$(if $(IFX_MULTILIB_UTIL),$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_util $(1)/usr/sbin/)
	$(if $(CONFIG_PACKAGE_ifx-IFXAPIs),\
		$(if $(CONFIG_TARGET_ltqcpe_platform_vbg400),,$(INSTALL_BIN) $(PKG_BUILD_DIR)/swreset $(1)/usr/sbin/;) \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/status_oper $(1)/usr/sbin/; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/next_macaddr $(1)/usr/sbin/; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/upgrade $(1)/usr/sbin/; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/syscfg_lock $(1)/usr/sbin/; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/ifx_event_util $(1)/usr/sbin/; \
	)
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/multicase_upgrade $(1)/usr/sbin/

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/free_dma $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mknod_util $(1)/usr/sbin/

	if [ A$(IFX_MULTILIB_UTIL) = Ay ]; then \
		cd $(1)/usr/sbin; \
			if [ ! -z $(CONFIG_PACKAGE_ifx-IFXAPIs) ]; then \
				ln -fs ifx_util get_atmqos_name; \
			fi; \
			ln -fs ifx_util get_if_index; \
			ln -fs ifx_util wdt; \
		if [ A$(CONFIG_PACKAGE_IFX_DSL_CPE_API) = Ay ]; then \
			ln -fs ifx_util get_adsl_rate; \
		fi; \
		if [ A$(CONFIG_FEATURE_IFX_DEBUG) = Ay ]; then \
			ln -fs ifx_util mem; \
			ln -fs ifx_util nohup; \
		fi; \
		if [ A$(CONFIG_FEATURE_IFX_VOIP) = Ay ]; then \
			ln -fs ifx_util voip_relay; \
			ln -fs ifx_util flash_timer; \
		fi; \
		ln -fs ifx_util read_img; \
		ln -fs ifx_util uboot_env; \
		if [ A$(CONFIG_FEATURE_LTQ_PARAMETER_ENCRYPTION) = Ay ]; then \
			ln -fs ifx_util ltq_crypt; \
		fi; \
		if [ A$(CONFIG_FEATURE_FIREWALL) = Ay -o A$(CONFIG_FEATURE_NAPT) = Ay ]; then \
			ln -fs ifx_util naptcfg; \
		fi; \
		if [ A$(CONFIG_FEATURE_IFX_IPQOS) = Ay ]; then \
			ln -fs ifx_util queuecfg; \
			ln -fs ifx_util qos_rate_update; \
		fi; \
		if [ ! -z $(CONFIG_PACKAGE_ifx-IFXAPIs) ]; then \
			ln -fs ifx_util route_util; \
		fi; \
		if [ A$(CONFIG_FEATURE_IFX_ADM6996_UTILITY) = Ay ]; then \
			ln -fs ifx_util adm6996; \
			ln -fs ifx_util vlan; \
		fi; \
		if [ A$(CONFIG_FEATURE_LQ_PPA_SESSION_MANAGEMENT) = Ay ]; then \
			ln -fs ifx_util ppasessmgmt; \
			$(INSTALL_BIN) $(PKG_BUILD_DIR)/ppasessmgmtd $(1)/usr/sbin/; \
		fi; \
		ln -fs ifx_util rcconf_chk; \
	elif [ A$(CONFIG_PACKAGE_ifx-IFXAPIs) != Ay ]; then \
		if [ A$(CONFIG_FEATURE_IFX_DEBUG) = Ay ]; then \
			$(INSTALL_BIN) $(PKG_BUILD_DIR)/mem $(1)/usr/sbin/; \
			$(INSTALL_BIN) $(PKG_BUILD_DIR)/nohup $(1)/usr/sbin/; \
		fi; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/get_if_index $(1)/usr/sbin/; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/usb_util $(1)/usr/sbin/; \
	fi
	$(INSTALL_DIR) $(1)/lib/modules/$(LINUX_VERSION)
	$(if $(CONFIG_TARGET_ltqcpe_platform_vbg400),,$(INSTALL_BIN) $(PKG_BUILD_DIR)/resetbutton/sw_reset.ko $(1)/lib/modules/$(LINUX_VERSION)/)
endef

define Build/Clean
	if [ -d $(PKG_BUILD_DIR) ]; then \
	  $(MAKE) -C $(AUTOGENCONF_DIR) clean; \
	  $(MAKE) -C $(PKG_BUILD_DIR)/pagebutton clean; \
	  $(MAKE) -C $(PKG_BUILD_DIR)/resetbutton clean; \
	  $(MAKE) -C $(PKG_BUILD_DIR) clean; \
	  $(RM) -r $(PKG_BUILD_DIR)/ipkg-$(BOARD); \
	fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
