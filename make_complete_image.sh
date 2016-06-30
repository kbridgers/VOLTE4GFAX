#!/bin/bash
dd if=/dev/zero of=CIOU_FLTE_SLIC_TEST_1_0_1.img bs=2M count=4
dd if=./root.squashfs of=CIOU_FLTE_SLIC_TEST_1_0_1.img bs=2M seek=1  conv=notrunc
dd if=bin/ltqcpe/vrx288_gw_he_vdsl_lte/uImage of=CIOU_FLTE_SLIC_TEST_1_0_1.img bs=2M conv=notrunc
