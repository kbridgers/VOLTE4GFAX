#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

I2C_MENU:=I2C support

ModuleConfVar=$(word 1,$(subst :,$(space),$(1)))
ModuleFullPath=$(if $(findstring y,$($(call ModuleConfVar,$(1)))),,$(LINUX_DIR)/$(word 2,$(subst :,$(space),$(1))).$(LINUX_KMOD_SUFFIX))
ModuleKconfig=$(foreach mod,$(1),$(call ModuleConfVar,$(mod)))
ModuleFiles=$(foreach mod,$(1),$(call ModuleFullPath,$(mod)))
ModuleAuto=$(call AutoLoad,$(1),$(foreach mod,$(2),$(basename $(notdir $(call ModuleFullPath,$(mod))))),$(3))

define i2c_defaults
  SUBMENU:=$(I2C_MENU)
  KCONFIG:=$(call ModuleKconfig,$(1))
  FILES:=$(call ModuleFiles,$(1))
  AUTOLOAD:=$(call ModuleAuto,$(2),$(1),$(3))
endef

I2C_CORE_MODULES:= \
  CONFIG_I2C:drivers/i2c/i2c-core \
  CONFIG_I2C_CHARDEV:drivers/i2c/i2c-dev

define KernelPackage/i2c-core
  $(call i2c_defaults,$(I2C_CORE_MODULES),51)
  TITLE:=I2C support
  DEPENDS:=@!LINUX_2_4
endef

define KernelPackage/i2c-core/description
 Kernel modules for I2C support
endef

$(eval $(call KernelPackage,i2c-core))


I2C_ALGOBIT_MODULES:= \
  CONFIG_I2C_ALGOBIT:drivers/i2c/algos/i2c-algo-bit

define KernelPackage/i2c-algo-bit
  $(call i2c_defaults,$(I2C_ALGOBIT_MODULES),55)
  TITLE:=I2C bit-banging interfaces
  DEPENDS:=kmod-i2c-core
endef

define KernelPackage/i2c-algo-bit/description
 Kernel modules for I2C bit-banging interfaces.
endef

$(eval $(call KernelPackage,i2c-algo-bit))


I2C_ALGOPCA_MODULES:= \
  CONFIG_I2C_ALGOPCA:drivers/i2c/algos/i2c-algo-pca

define KernelPackage/i2c-algo-pca
  $(call i2c_defaults,$(I2C_ALGOPCA_MODULES),55)
  TITLE:=I2C PCA 9564 interfaces
  DEPENDS:=kmod-i2c-core
endef

define KernelPackage/i2c-algo-pca/description
 Kernel modules for I2C PCA 9564 interfaces.
endef

$(eval $(call KernelPackage,i2c-algo-pca))


I2C_ALGOPCF_MODULES:= \
  CONFIG_I2C_ALGOPCF:drivers/i2c/algos/i2c-algo-pcf

define KernelPackage/i2c-algo-pcf
  $(call i2c_defaults,$(I2C_ALGOPCF_MODULES),55)
  TITLE:=I2C PCF 8584 interfaces
  DEPENDS:=kmod-i2c-core
endef

define KernelPackage/i2c-algo-pcf/description
 Kernel modules for I2C PCF 8584 interfaces
endef

$(eval $(call KernelPackage,i2c-algo-pcf))


I2C_GPIO_MODULES:= \
  CONFIG_I2C_GPIO:drivers/i2c/busses/i2c-gpio

define KernelPackage/i2c-gpio
  $(call i2c_defaults,$(I2C_GPIO_MODULES),59)
  TITLE:=GPIO-based bitbanging I2C
  DEPENDS:=@GPIO_SUPPORT +kmod-i2c-algo-bit
endef

define KernelPackage/i2c-gpio/description
 Kernel modules for a very simple bitbanging I2C driver utilizing the
 arch-neutral GPIO API to control the SCL and SDA lines.
endef

$(eval $(call KernelPackage,i2c-gpio))

I2C_SCX200_MODULES:=\
  CONFIG_SCx200_I2C:drivers/i2c/busses/scx200_i2c

define KernelPackage/i2c-scx200
  $(call i2c_defaults,$(I2C_SCX200_MODULES),59)
  TITLE:=Geode SCx200 I2C using GPIO pins
  DEPENDS:=@PCI_SUPPORT @TARGET_x86 +kmod-i2c-algo-bit
  KCONFIG+= \
	CONFIG_SCx200_I2C_SCL=12 \
	CONFIG_SCx200_I2C_SDA=13
endef

define KernelPackage/i2c-scx200/description
 Kernel module for I2C using GPIO pins on the Geode SCx200 processors.
endef

$(eval $(call KernelPackage,i2c-scx200))


I2C_SCX200_ACB_MODULES:=\
  CONFIG_SCx200_ACB:drivers/i2c/busses/scx200_acb

define KernelPackage/i2c-scx200-acb
  $(call i2c_defaults,$(I2C_SCX200_ACB_MODULES),59)
  TITLE:=Geode SCx200 ACCESS.bus support
  DEPENDS:=@PCI_SUPPORT @TARGET_x86 +kmod-i2c-algo-bit
endef

define KernelPackage/i2c-scx200-acb/description
 Kernel module for I2C using the ACCESS.bus controllers on the Geode SCx200
 and SC1100 processors and the CS5535 and CS5536 Geode companion devices.
endef

$(eval $(call KernelPackage,i2c-scx200-acb))


OF_I2C_MODULES:=\
  CONFIG_OF_I2C:drivers/of/of_i2c

define KernelPackage/of-i2c
  $(call i2c_defaults,$(OF_I2C_MODULES),58)
  TITLE:=OpenFirmware I2C accessors
  DEPENDS:=@TARGET_ppc40x||TARGET_ppc4xx kmod-i2c-core
endef

define KernelPackage/of-i2c/description
 Kernel module for OpenFirmware I2C accessors.
endef

$(eval $(call KernelPackage,of-i2c))


I2C_IBM_IIC_MODULES:=\
  CONFIG_I2C_IBM_IIC:drivers/i2c/busses/i2c-ibm_iic

define KernelPackage/i2c-ibm-iic
  $(call i2c_defaults,$(OF_I2C_MODULES),59)
  TITLE:=IBM PPC 4xx on-chip I2C interface support
  DEPENDS:=@TARGET_ppc40x||TARGET_ppc4xx +kmod-i2c-core +kmod-of-i2c
endef

define KernelPackage/i2c-ibm-iic/description
 Kernel module for IIC peripheral found on embedded IBM PPC4xx based systems.
endef

$(eval $(call KernelPackage,i2c-ibm-iic))
