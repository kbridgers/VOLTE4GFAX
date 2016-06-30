# All Target
#all: LTEManager
#/home/vvdnlt230/vvdn_all/ciou_flte/software/workspace/ugw5_4_workspace/UGW-5.4-SW-CD/UGW-5.4-SW-CD/Software/Sources/UGW-5.4/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin/mips-openwrt-linux-uclibc-gcc 
# Tool invocations
LTEManager: 
	@echo 'Building target: $@'
	@echo 'Invoking: cross compiler'
	 $(TOPDIR)/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin/mips-openwrt-linux-uclibc-gcc -g main.c comprocess.c error_defines.c ping.c ATCommands.c  -I./ -I./../inc/ -o "LTEManager" 
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM)  LTEManager
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:


