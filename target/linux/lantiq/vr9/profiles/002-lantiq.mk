define Profile/EASY80920
  NAME:=EASY80920
  PACKAGES:=kmod-leds-gpio button-hotplug kmod-usb-ifxhcd
endef

$(eval $(call Profile,EASY80920))
