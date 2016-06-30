#!/bin/sh /etc/rc.common

#START=25

start() {
#	kernel_version=`uname -r`
	/sbin/insmod /lib/modules/*/ifx_nfext_core.ko
	/sbin/insmod /lib/modules/*/ifx_nfext_ppp.ko
   /sbin/insmod /lib/modules/*/nf_conntrack_proto_esp.ko
}
