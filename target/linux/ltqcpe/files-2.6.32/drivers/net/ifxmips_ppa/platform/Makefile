obj-$(CONFIG_AR9)	+= ar9/
ifneq ($(CONFIG_IFX_PPA_VRX218_E1),)
  obj-$(CONFIG_AR10)	+= vrx218/
else
 ifneq ($(CONFIG_IFX_PPA_VRX218_A1),)
  obj-$(CONFIG_AR10)	+= vrx218/
 else
  obj-$(CONFIG_VR9)	+= vr9/
  obj-$(CONFIG_AR10)	+= ar10/
 endif
endif
