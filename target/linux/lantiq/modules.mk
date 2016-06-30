#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

CRYPTO_MENU:=Cryptographic API modules

define KernelPackage/lantiq-deu
  TITLE:=Lantiq data encryption unit
  SUBMENU:=$(CRYPTO_MENU)
  DEPENDS:=@TARGET_lantiq
  KCONFIG:=CONFIG_CRYPTO_DEV_LANTIQ \
	   CONFIG_CRYPTO_HW=y \
	   CONFIG_CRYPTO_DEV_LANTIQ_AES=y \
	   CONFIG_CRYPTO_DEV_LANTIQ_DES=y \
	   CONFIG_CRYPTO_DEV_LANTIQ_MD5=y \
	   CONFIG_CRYPTO_DEV_LANTIQ_SHA1=y
  DEPENDS+=@TARGET_lantiq +kmod-crypto-core
endef

define KernelPackage/lantiq-deu/description
  Kernel support for the Lantiq crypto HW
endef

$(eval $(call KernelPackage,lantiq-deu))

USB_MENU:=USB Support

define KernelPackage/usb-dwc-otg
  TITLE:=Synopsis DWC_OTG support
  SUBMENU:=$(USB_MENU)
  DEPENDS+=@TARGET_lantiq_danube +kmod-usb-core
  KCONFIG:=CONFIG_DWC_OTG \
  	CONFIG_DWC_OTG_DEBUG=n \
	CONFIG_DWC_OTG_LANTIQ=y \
	CONFIG_DWC_OTG_HOST_ONLY=y \
	CONFIG_DWC_OTG_DEVICE_ONLY=n
  FILES:=$(LINUX_DIR)/drivers/usb/dwc_otg/dwc_otg.ko
  AUTOLOAD:=$(call AutoLoad,50,dwc_otg)
endef

define KernelPackage/usb-dwc-otg/description
  Kernel support for Synopsis USB on XWAY
endef

$(eval $(call KernelPackage,usb-dwc-otg))

I2C_LANTIQ_MODULES:= \
  CONFIG_I2C_LANTIQ:drivers/i2c/busses/i2c-lantiq

define KernelPackage/i2c-lantiq
  TITLE:=Falcon I2C controller
  SUBMENU:=I2C support
  DEPENDS:=kmod-i2c-core @TARGET_lantiq
  KCONFIG:=CONFIG_I2C_LANTIQ
  FILES:=$(LINUX_DIR)/drivers/i2c/busses/i2c-lantiq.ko
  AUTOLOAD:=$(call AutoLoad,52,i2c-lantiq)
endef

define KernelPackage/i2c-lantiq/description
  Kernel support for the Lantiq I2C controller
endef

$(eval $(call KernelPackage,i2c-lantiq))

define KernelPackage/lantiq-vpe
  TITLE:=Lantiq VPE extensions
  SUBMENU:=Lantiq
  DEPENDS:=@TARGET_lantiq +kmod-vpe
  KCONFIG:=CONFIG_IFX_VPE_CACHE_SPLIT=y \
	  CONFIG_IFX_VPE_EXT=y \
	  CONFIG_VPE_SOFTDOG=n \
	  CONFIG_MTSCHED=y \
	  CONFIG_PERFCTRS=n
endef

define KernelPackage/lantiq-vpe/description
  Kernel extensions for the Lantiq SoC
endef

$(eval $(call KernelPackage,lantiq-vpe))

