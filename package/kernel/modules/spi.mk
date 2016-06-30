#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

SPI_MENU:=SPI Support

define KernelPackage/mmc-spi
  SUBMENU:=$(SPI_MENU)
  TITLE:=MMC/SD over SPI Support
  DEPENDS:=@!LINUX_2_4 +kmod-mmc +kmod-crc-itu-t +kmod-crc7
  KCONFIG:=CONFIG_MMC_SPI \
          CONFIG_SPI=y \
          CONFIG_SPI_MASTER=y
  FILES:=$(LINUX_DIR)/drivers/mmc/host/mmc_spi.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,90,mmc_spi)
endef

define KernelPackage/mmc-spi/description
 Kernel support for MMC/SD over SPI
endef

$(eval $(call KernelPackage,mmc-spi))


define KernelPackage/spi-bitbang
  SUBMENU:=$(SPI_MENU)
  TITLE:=Serial Peripheral Interface bitbanging library
  DEPENDS:=@!LINUX_2_4
  KCONFIG:=CONFIG_SPI_BITBANG \
          CONFIG_SPI=y \
          CONFIG_SPI_MASTER=y
  FILES:=$(LINUX_DIR)/drivers/spi/spi_bitbang.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,91,spi_bitbang)
endef

define KernelPackage/spi-bitbang/description
 This package contains the SPI bitbanging library
endef

$(eval $(call KernelPackage,spi-bitbang))


define KernelPackage/spi-gpio-old
  SUBMENU:=$(SPI_MENU)
  TITLE:=Old GPIO based bitbanging SPI controller (DEPRECATED)
  DEPENDS:=@GPIO_SUPPORT +kmod-spi-bitbang
  KCONFIG:=CONFIG_SPI_GPIO_OLD
  FILES:=$(LINUX_DIR)/drivers/spi/spi_gpio_old.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,92,spi_gpio_old)
endef

define KernelPackage/spi-gpio-old/description
 This package contains the GPIO based bitbanging SPI controller driver
endef

$(eval $(call KernelPackage,spi-gpio-old))

define KernelPackage/spi-gpio
  SUBMENU:=$(SPI_MENU)
  TITLE:=GPIO-based bitbanging SPI Master
  DEPENDS:=@GPIO_SUPPORT +kmod-spi-bitbang
  KCONFIG:=CONFIG_SPI_GPIO
  FILES:=$(LINUX_DIR)/drivers/spi/spi_gpio.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,92,spi_gpio)
endef

define KernelPackage/spi-gpio/description
 This package contains the GPIO-based bitbanging SPI Master
endef

$(eval $(call KernelPackage,spi-gpio))

define KernelPackage/spi-dev
  SUBMENU:=$(SPI_MENU)
  TITLE:=User mode SPI device driver
  DEPENDS:=@!LINUX_2_4
  KCONFIG:=CONFIG_SPI_SPIDEV \
          CONFIG_SPI=y \
          CONFIG_SPI_MASTER=y
  FILES:=$(LINUX_DIR)/drivers/spi/spidev.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,93,spidev)
endef

define KernelPackage/spi-dev/description
 This package contains the user mode SPI device driver
endef

$(eval $(call KernelPackage,spi-dev))

define KernelPackage/bcm63xx-spi
  SUBMENU:=$(SPI_MENU)
  TITLE:=Broadcom BCM63xx SPI driver
  DEPENDS:=@TARGET_brcm63xx +kmod-spi-bitbang
  KCONFIG:=CONFIG_SPI_BCM63XX
  FILES:=$(LINUX_DIR)/drivers/spi/bcm63xx_spi.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,92,bcm63xx_spi)
endef

define KernelPackage/bcm63xx-spi/description
  This package contains the Broadcom BCM63xx SPI Master driver
endef

$(eval $(call KernelPackage,bcm63xx-spi))


define KernelPackage/spi-vsc7385
  SUBMENU:=$(SPI_MENU)
  TITLE:=Vitesse VSC7385 ethernet switch driver
  DEPENDS:=@TARGET_ar71xx
  KCONFIG:=CONFIG_SPI_VSC7385
  FILES:=$(LINUX_DIR)/drivers/spi/spi_vsc7385.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,93,spi_vsc7385)
endef

define KernelPackage/spi-vsc7385/description
  This package contains the SPI driver for the Vitesse VSC7385 ethernet switch.
endef

$(eval $(call KernelPackage,spi-vsc7385))
