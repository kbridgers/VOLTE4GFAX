# Makefile for OpenWrt
#
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

TOPDIR:=${CURDIR}
LC_ALL:=C
LANG:=C
export TOPDIR LC_ALL LANG

ifndef __mxl_config_mi

# Inclusion guard for this file.
__mxl_config_mi:=$(true)

# This variable represents the true construct.
true:=true

# This tests the existance of $(lastword) (GNU Make 3.81).
__mxl_have_lastword:=$(lastword $(true))

# This defines the make compatibility for GNU Make 3.81.
__mxl_have_make_3.81:=$(__mxl_have_lastword)

endif # __mxl_config_mi

ifndef __mxl_have_make_3.81

world all %: $(TOPDIR)/staging_dir/host/bin/make
	@+if [ -f $(TOPDIR)/staging_dir/host/bin/make ]; then \
		$(TOPDIR)/staging_dir/host/bin/make -C $(TOPDIR) $@; \
	else \
		echo "ERROR: make executable not found ($(TOPDIR)/staging_dir/host/bin/make) "; \
		rm $(TOPDIR)/staging_dir/host/stamp/.make; \
		false; \
	fi

$(TOPDIR)/staging_dir/host/bin/make:
	@$(MAKE) -f $(TOPDIR)/include/prereq-make-install.mk

.PHONY: world all %

else

world:

include $(TOPDIR)/include/host.mk

ifneq ($(OPENWRT_BUILD),1)
  # XXX: these three lines are normally defined by rules.mk
  # but we can't include that file in this context
  empty:=
  space:= $(empty) $(empty)
  _SINGLE=export MAKEFLAGS=$(space);

  override OPENWRT_BUILD=1
  export OPENWRT_BUILD
  include $(TOPDIR)/include/debug.mk
  include $(TOPDIR)/include/depends.mk
  include $(TOPDIR)/include/toplevel.mk
else
  include rules.mk
  include $(INCLUDE_DIR)/depends.mk
  include $(INCLUDE_DIR)/subdir.mk
  include target/Makefile
  include package/Makefile
  include tools/Makefile
  include toolchain/Makefile
$(toolchain/stamp-install): $(tools/stamp-install)
$(target/stamp-compile): $(toolchain/stamp-install) $(tools/stamp-install) $(BUILD_DIR)/.prepared
$(package/stamp-cleanup): $(target/stamp-compile)
$(package/stamp-compile): $(target/stamp-compile) $(package/stamp-cleanup)
$(package/stamp-install): $(package/stamp-compile)
$(package/stamp-rootfs-prepare): $(package/stamp-install)
$(target/stamp-install): $(package/stamp-compile) $(package/stamp-install) $(package/stamp-rootfs-prepare) 
$(BUILD_DIR)/.prepared: Makefile
	@mkdir -p $$(dirname $@)
	@touch $@

prepare: $(target/stamp-compile)

clean: FORCE
	$(_SINGLE)$(SUBMAKE) target/linux/clean
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(BUILD_LOG_DIR)

dirclean: clean
	rm -rf $(STAGING_DIR) $(STAGING_DIR_HOST) $(STAGING_DIR_TOOLCHAIN) $(TOOLCHAIN_DIR) $(BUILD_DIR_HOST) $(BUILD_DIR_TOOLCHAIN)
	rm -rf $(TMP_DIR)

tmp/.prereq_packages: .config
	unset ERROR; \
	for package in $(sort $(prereq-y) $(prereq-m)); do \
		$(_SINGLE)$(NO_TRACE_MAKE) -s -r -C package/$$package prereq || ERROR=1; \
	done; \
	if [ -n "$$ERROR" ]; then \
		echo "Package prerequisite check failed."; \
		false; \
	fi
	touch $@

# check prerequisites before starting to build
prereq: $(target/stamp-prereq) tmp/.prereq_packages vvdnprereqs

prepare: .config $(tools/stamp-install) $(toolchain/stamp-install)
world: prepare $(target/stamp-compile) $(package/stamp-cleanup) $(package/stamp-compile) $(package/stamp-install) $(package/stamp-rootfs-prepare) $(target/stamp-install) FORCE
	$(_SINGLE)$(SUBMAKE) -r package/index

# update all feeds, re-create index files, install symlinks
package/symlinks:
	$(SCRIPT_DIR)/feeds update -a
	$(SCRIPT_DIR)/feeds install -a

# re-create index files, install symlinks
package/symlinks-install:
	$(SCRIPT_DIR)/feeds update -i
	$(SCRIPT_DIR)/feeds install -a

# remove all symlinks, don't touch ./feeds
package/symlinks-clean:
	$(SCRIPT_DIR)/feeds uninstall -a

vvdnprereqs: #$(TOPDIR)/staging_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/usr/include/drv_ter1x66/73m1966_io.h
	@mkdir -p $(TOPDIR)/staging_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/firmware
	@mkdir -p $(TOPDIR)/staging_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/usr/include/drv_ter1x66
	cp $(TOPDIR)/vvdn/73m1966_io.h $(TOPDIR)/staging_dir/target-mips_r2_uClibc-0.9.30.1_vrx288_gw_he_vdsl_lte/usr/include/drv_ter1x66/.

.PHONY: clean dirclean prereq prepare world package/symlinks package/symlinks-install package/symlinks-clean

endif

endif # 3.81 compatibility check
