# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=FLTE_I2CApp
PKG_VERSION:=1_0_1
PKG_RELEASE:=1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)
#PKG_SOURCE_URL:=http://www.zlib.net @SF/zlib
#PKG_MD5SUM:=dee233bf288ee795ac96a98cc2e369b6

include $(INCLUDE_DIR)/package.mk

define Package/FLTE_I2CApp
  SECTION:=I2C Application
  CATEGORY:=FAX over LTE
  TITLE:=ciou_flte I2C communication application
  URL:=ConnectedIO
endef

#define Build/Configure
#	(cd $(PKG_BUILD_DIR); \
#		$(TARGET_CONFIGURE_OPTS) \
##		LDSHARED="$(TARGET_CC) -shared -Wl,-soname,libz.so.1" \
#		CFLAGS="$(TARGET_CFLAGS) $(FPIC)" \
#		UNAME_S="Linux" \
#		./configure \
#			--prefix=/usr \
#			--shared \
#	);
#endef
ifdef CONFIG_LTE_MANAGER_APPLICATION_IN_ROOTFS
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)

endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) 
endef


# libz.so is needed for openssl (zlib-dynamic)
define Package/FLTE_I2CApp/install
	mkdir -p $(PKG_INSTALL_DIR)/usr/sbin
	mkdir -p $(BUILD_DIR)/root-ltqcpe/usr/sbin
	$(CP) $(PKG_BUILD_DIR)/I2CApp_hex $(PKG_INSTALL_DIR)/usr/sbin/I2CApp_hex
	$(CP) $(PKG_BUILD_DIR)/I2CApp_hex $(BUILD_DIR)/root-ltqcpe/usr/sbin/I2CApp_hex
	mkdir -p $(BUILD_DIR)/root-ltqcpe/etc/init.d
	$(CP) ./files/configurecodec $(BUILD_DIR)/root-ltqcpe/usr/sbin
	$(CP) ./files/resetcodecconfig.sh $(BUILD_DIR)/root-ltqcpe/etc/init.d
endef
endif
$(eval $(call BuildPackage,FLTE_I2CApp))
