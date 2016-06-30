#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=FLTE_led_driver
PKG_RELEASE:=2

include $(INCLUDE_DIR)/package.mk

define KernelPackage/FLTE_led_driver
  SUBMENU:=vvdn
  TITLE:=led driver
  FILES:=$(PKG_BUILD_DIR)/FLTE_led_driver.ko
  KCONFIG:=
endef

define KernelPackage/FLTE_led_driver/description
 Kernel module for led driver.
endef

EXTRA_KCONFIG:= \
	CONFIG_FLTE_led_driver=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef
define KernelPackage/$(PKG_NAME)/install
	mkdir -p $(BUILD_DIR)/root-ltqcpe/lib/modules/2.6.32.42 
	$(CP) $(PKG_BUILD_DIR)/FLTE_led_driver.ko $(BUILD_DIR)/root-ltqcpe/lib/modules/2.6.32.42
	mkdir -p $(BUILD_DIR)/root-ltqcpe/etc/init.d
	$(CP) ./files/node_led.sh $(BUILD_DIR)/root-ltqcpe/etc/init.d/
endef

$(eval $(call KernelPackage,FLTE_led_driver))
