# All Target
all: I2CApp
#/home/vvdnlt230/vvdn_all/ciou_flte/software/workspace/ugw5_4_workspace/UGW-5.4-SW-CD/UGW-5.4-SW-CD/Software/Sources/UGW-5.4/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin/mips-openwrt-linux-uclibc-gcc 
# Tool invocations
I2CApp: 
	@echo 'Building target: $@'
	@echo 'Invoking: cross compiler'
	 $(TOPDIR)/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin/mips-openwrt-linux-uclibc-gcc -g led_updater.c -I./ -o "LEDUpdater" 
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM)  I2CApp
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:


