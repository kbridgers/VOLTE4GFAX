#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

override TARGET_BUILD=
include $(INCLUDE_DIR)/prereq.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/host.mk

.NOTPARALLEL:
override MAKEFLAGS=
override MAKE:=$(SUBMAKE)
KDIR=$(KERNEL_BUILD_DIR)

IMG_PREFIX:=openwrt-$(BOARD)$(if $(SUBTARGET),-$(SUBTARGET))

ifneq ($(CONFIG_BIG_ENDIAN),)
  JFFS2OPTS     :=  --pad --big-endian --squash -v
  SQUASHFS_OPTS :=  -be
else
  JFFS2OPTS     :=  --pad --little-endian --squash -v
  SQUASHFS_OPTS :=  -le
endif

ifeq ($(CONFIG_JFFS2_RTIME),y)
  JFFS2OPTS += -X rtime
endif
ifeq ($(CONFIG_JFFS2_ZLIB),y)
  JFFS2OPTS += -X zlib
endif
ifeq ($(CONFIG_JFFS2_LZMA),y)
  JFFS2OPTS += -X lzma --compression-mode=size
endif
ifneq ($(CONFIG_JFFS2_RTIME),y)
  JFFS2OPTS +=  -x rtime
endif
ifneq ($(CONFIG_JFFS2_ZLIB),y)
  JFFS2OPTS += -x zlib
endif
ifneq ($(CONFIG_JFFS2_LZMA),y)
  JFFS2OPTS += -x lzma
endif

ifneq ($(CONFIG_LINUX_2_4)$(CONFIG_LINUX_2_6_25),)
  USE_SQUASHFS3 := y
endif

ifneq ($(USE_SQUASHFS3),)
  MKSQUASHFS_CMD := $(STAGING_DIR_HOST)/bin/mksquashfs-lzma
else
  MKSQUASHFS_CMD := $(STAGING_DIR_HOST)/bin/mksquashfs4
  SQUASHFS_OPTS  := -comp lzma -processors 1
endif

JFFS2_BLOCKSIZE ?= 64k 128k

define add_jffs2_mark
	echo -ne '\xde\xad\xc0\xde' >> $(1)
endef

# pad to 4k, 8k, 16k, 64k, 128k, 256k and add jffs2 end-of-filesystem mark
define prepare_generic_squashfs
	dd if=$(1) of=$(KDIR)/tmpfile.0 bs=4k conv=sync
	$(call add_jffs2_mark,$(KDIR)/tmpfile.0)
	dd if=$(KDIR)/tmpfile.0 of=$(KDIR)/tmpfile.1 bs=4k conv=sync
	$(call add_jffs2_mark,$(KDIR)/tmpfile.1)
	dd if=$(KDIR)/tmpfile.1 of=$(KDIR)/tmpfile.2 bs=8k conv=sync
	$(call add_jffs2_mark,$(KDIR)/tmpfile.2)
	dd if=$(KDIR)/tmpfile.2 of=$(KDIR)/tmpfile.3 bs=64k conv=sync
	$(call add_jffs2_mark,$(KDIR)/tmpfile.3)
	dd if=$(KDIR)/tmpfile.3 of=$(KDIR)/tmpfile.4 bs=64k conv=sync
	$(call add_jffs2_mark,$(KDIR)/tmpfile.4)
	dd if=$(KDIR)/tmpfile.4 of=$(1) bs=128k conv=sync
	$(call add_jffs2_mark,$(1))
	rm -f $(KDIR)/tmpfile.*
endef


ifneq ($(CONFIG_TARGET_ROOTFS_INITRAMFS),)

  define Image/BuildKernel
		cp $(KDIR)/vmlinux.elf $(BIN_DIR)/$(IMG_PREFIX)-vmlinux.elf
		$(call Image/Build/Initramfs)
  endef

else

  ifneq ($(CONFIG_TARGET_ROOTFS_JFFS2),)
    define Image/mkfs/jffs2/sub
		# FIXME: removing this line will cause strange behaviour in the foreach loop below
		$(STAGING_DIR_HOST)/bin/mkfs.jffs2 $(JFFS2OPTS) -e $(patsubst %k,%KiB,$(1)) -o $(KDIR)/root.jffs2-$(1) -d $(TARGET_DIR) -v 2>&1 1>/dev/null | awk '/^.+$$$$/'
		$(call add_jffs2_mark,$(KDIR)/root.jffs2-$(1))
		$(call Image/Build,jffs2-$(1))
    endef
    define Image/mkfs/jffs2
		$(foreach SZ,$(JFFS2_BLOCKSIZE),$(call Image/mkfs/jffs2/sub,$(SZ)))
    endef
  endif

  ifneq ($(CONFIG_TARGET_ROOTFS_SQUASHFS),)
    define Image/mkfs/squashfs
		@mkdir -p $(TARGET_DIR)/overlay
		$(MKSQUASHFS_CMD) $(TARGET_DIR) $(KDIR)/root.squashfs -nopad -noappend -root-owned $(SQUASHFS_OPTS)
		$(call Image/Build,squashfs)
    endef
  endif

  ifneq ($(CONFIG_TARGET_ROOTFS_UBIFS),)
    define Image/mkfs/ubifs
		$(CP) ./ubinize.cfg $(KDIR)
		$(STAGING_DIR_HOST)/bin/mkfs.ubifs $(UBIFS_OPTS) -o $(KDIR)/root.ubifs -d $(TARGET_DIR)
		(cd $(KDIR); \
		$(STAGING_DIR_HOST)/bin/ubinize $(UBINIZE_OPTS) -o $(KDIR)/root.ubi ubinize.cfg)
		$(call Image/Build,ubi)
    endef
  endif

endif

ifneq ($(CONFIG_TARGET_ROOTFS_CPIOGZ),)
  define Image/mkfs/cpiogz
		( cd $(TARGET_DIR); find . | cpio -o -H newc | gzip -9 >$(BIN_DIR)/$(IMG_PREFIX)-rootfs.cpio.gz )
  endef
endif

ifneq ($(CONFIG_TARGET_ROOTFS_TARGZ),)
  define Image/mkfs/targz
		$(TAR) -zcf $(BIN_DIR)/$(IMG_PREFIX)-rootfs.tar.gz --numeric-owner --owner=0 --group=0 -C $(TARGET_DIR)/ .
  endef
endif

ifneq ($(CONFIG_TARGET_ROOTFS_EXT2FS),)
  E2SIZE=$(shell echo $$(($(CONFIG_TARGET_ROOTFS_PARTSIZE)*1024)))

  define Image/mkfs/ext2
		$(STAGING_DIR_HOST)/bin/genext2fs -U -b $(E2SIZE) -N $(CONFIG_TARGET_ROOTFS_MAXINODE) -d $(TARGET_DIR)/ $(KDIR)/root.ext2
		$(call Image/Build,ext2)
  endef
endif

ifneq ($(CONFIG_TARGET_ROOTFS_ISO),)
  define Image/mkfs/iso
		$(call Image/Build,iso)
  endef
endif


define Image/mkfs/prepare/default
	- $(FIND) $(TARGET_DIR) -type f -not -perm +0100 -not -name 'ssh_host*' -print0 | $(XARGS) -0 chmod 0644
	- $(FIND) $(TARGET_DIR) -type f -perm +0100 -print0 | $(XARGS) -0 chmod 0755
	- $(FIND) $(TARGET_DIR) -type d -print0 | $(XARGS) -0 chmod 0755
	$(INSTALL_DIR) $(TARGET_DIR)/tmp
	chmod 0777 $(TARGET_DIR)/tmp
endef

define Image/mkfs/prepare
	$(call Image/mkfs/prepare/default)
	$(call Image/mkfs/prepare/platform)
endef


define Image/Checksum
	( cd ${BIN_DIR} ; \
		$(FIND) -maxdepth 2 -type f \! -name 'md5sums'  -printf "%P\n" | sort | xargs \
		md5sum --binary > md5sums \
	)
endef


define BuildImage

  download:
  prepare:

  ifeq ($(IB),)
    compile: compile-targets FORCE
		$(call Build/Compile)
  else
    compile:
  endif

  ifeq ($(IB),)
    install: compile install-targets FORCE
		$(call Image/Prepare)
		$(call Image/mkfs/prepare)
		$(call Image/BuildKernel)
		$(call Image/mkfs/cpiogz)
		$(call Image/mkfs/targz)
		$(call Image/mkfs/ext2)
		$(call Image/mkfs/iso)
		$(call Image/mkfs/jffs2)
		$(call Image/mkfs/squashfs)
		$(call Image/mkfs/ubifs)
		$(call Image/mkfs/platform)
		$(call Image/Checksum)
  else
    install: compile install-targets
		$(call Image/BuildKernel)
		$(call Image/mkfs/cpiogz)
		$(call Image/mkfs/targz)
		$(call Image/mkfs/ext2)
		$(call Image/mkfs/iso)
		$(call Image/mkfs/jffs2)
		$(call Image/mkfs/squashfs)
		$(call Image/mkfs/ubifs)
		$(call Image/mkfs/platform)
		$(call Image/Checksum)
  endif

  ifeq ($(IB),)
    clean: clean-targets
		$(call Build/Clean)
  else
    clean:
  endif

  compile-targets:
  install-targets:
  clean-targets:

endef
