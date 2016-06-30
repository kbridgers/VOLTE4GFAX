include $(TOPDIR)/rules.mk

PKG_NAME:=arp-scan
PKG_VERSION:=1.8.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.nta-monitor.com/tools/arp-scan/download/
PKG_MD5SUM:=38db8f27fc6553a88367748ea04483d6

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=net
	CATEGORY:=Network
	TITLE:=Support for  ARP-SCAN
	URL:=http://www.nta-monitor.com/tools/arp-scan
	DEPENDS:=+libpcap
endef

define Package/$(PKG_NAME)/description
	Package for ARP-SCAN Support 
endef

CONFIGURE_ARGS	+= --libdir=$(STAGING_DIR)/usr/lib --includedir=$(STAGING_DIR)/usr/include/pcap LIBS=-lpcap

export STAGING_DIR

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) 
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_BUILD_DIR)/arp-scan $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
