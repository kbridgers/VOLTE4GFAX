#
# Copyright (C) 2006,2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

HWMON_MENU:=Hardware Monitoring Support

define KernelPackage/hwmon-core
  SUBMENU:=$(HWMON_MENU)
  TITLE:=Hardware monitoring support
  DEPENDS:=@!LINUX_2_4
  KCONFIG:= \
	CONFIG_HWMON \
	CONFIG_HWMON_DEBUG_CHIP=n
  FILES:= \
	$(LINUX_DIR)/drivers/hwmon/hwmon.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,40,hwmon)
endef

define KernelPackage/hwmon-core/description
 Kernel modules for hardware monitoring
endef

$(eval $(call KernelPackage,hwmon-core))


define KernelPackage/hwmon/Depends
  SUBMENU:=$(HWMON_MENU)
  DEPENDS:=kmod-hwmon-core $(1)
endef


define KernelPackage/hwmon-lm75
$(call KernelPackage/hwmon/Depends,+kmod-i2c-core)
  TITLE:=LM75 monitoring support
  KCONFIG:=CONFIG_SENSORS_LM75
  FILES:=$(LINUX_DIR)/drivers/hwmon/lm75.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,60,lm75)
endef

define KernelPackage/hwmon-lm75/description
 Kernel module for lm75 thermal monitor chip
endef

$(eval $(call KernelPackage,hwmon-lm75))


define KernelPackage/hwmon-lm77
$(call KernelPackage/hwmon/Depends,+kmod-i2c-core)
  TITLE:=LM77 monitoring support
  KCONFIG:=CONFIG_SENSORS_LM77
  FILES:=$(LINUX_DIR)/drivers/hwmon/lm77.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,60,lm77)
endef

define KernelPackage/hwmon-lm77/description
 Kernel module for LM77 thermal monitor chip
endef

$(eval $(call KernelPackage,hwmon-lm77))

define KernelPackage/hwmon-lm90
$(call KernelPackage/hwmon/Depends,+kmod-i2c-core)
  TITLE:=LM90 monitoring support
  KCONFIG:=CONFIG_SENSORS_LM90
  FILES:=$(LINUX_DIR)/drivers/hwmon/lm90.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,60,lm90)
endef

define KernelPackage/hwmon-lm90/description
 Kernel module for LM90 thermal monitor chip
endef

$(eval $(call KernelPackage,hwmon-lm90))

define KernelPackage/hwmon-pc87360
$(call KernelPackage/hwmon/Depends,@TARGET_x86)
  TITLE:=PC87360 monitoring support
  KCONFIG:= \
	CONFIG_SENSORS_PC87360 \
	CONFIG_HWMON_VID
  FILES:= \
	$(LINUX_DIR)/drivers/hwmon/hwmon-vid.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/hwmon/pc87360.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,50,hwmon-vid pc87360)
endef

define KernelPackage/hwmon-pc87360/description
 Kernel modules for PC87360 chips
endef

$(eval $(call KernelPackage,hwmon-pc87360))

define KernelPackage/hwmon-w83627hf
$(call KernelPackage/hwmon/Depends,@TARGET_rdc||TARGET_x86)
  TITLE:=Winbond W83627HF monitoring support
  KCONFIG:= \
	CONFIG_SENSORS_W83627HF \
	CONFIG_HWMON_VID
  FILES:= \
	$(LINUX_DIR)/drivers/hwmon/hwmon-vid.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/hwmon/w83627hf.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,50,hwmon-vid w83627hf)
endef

define KernelPacakge/hwmon-w83627hf/description
  Kernel module for the Winbond W83627HF chips.
endef

$(eval $(call KernelPackage,hwmon-w83627hf))
