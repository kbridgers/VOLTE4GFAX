



#
#                              Copyright (c) 2012
#                            Lantiq Deutschland GmbH
#
#  For licensing information, see the file 'LICENSE' in the root folder of
#  this software module.
#

if TAPIDEMO_LIBRARY
lib_LIBRARIES = libtapidemo.a
else
bin_PROGRAMS = tapidemo
endif

include Makefile.inc

nodist_tapidemo_SOURCES =

## tapi demo program
common_SRC = \
    @srcdir@/common.c \
    @srcdir@/lib_tapi_signal.c \
    @srcdir@/tapidemo.c \
    @srcdir@/qos.c \
    @srcdir@/cid.c \
    @srcdir@/conference.c \
    @srcdir@/abstract.c \
    @srcdir@/pcm.c \
    @srcdir@/voip.c \
    @srcdir@/feature.c \
    @srcdir@/analog.c \
    @srcdir@/parse_cmd_arg.c \
    @srcdir@/td_t38.c \
    @srcdir@/state_trans.c \
    @srcdir@/event_handling_msg.c \
    @srcdir@/td_ifxos_map.c \
    @srcdir@/td_socket.c \
    @srcdir@/itm/com_client.c \
    @srcdir@/itm/control_pc.c \
    @srcdir@/itm/dev_help.c

if SYS_EASY3332
common_SRC += \
    @srcdir@/board_easy3332.c \
    @srcdir@/device_vinetic_cpe.c
endif

if SYS_DANUBE
common_SRC += \
    @srcdir@/board_easy50712.c

if DEV_VINETIC
common_SRC += \
    @srcdir@/board_easy50510.c \
    @srcdir@/device_vinetic_cpe.c
endif

endif

if DEV_DXT
common_SRC += \
    @srcdir@/board_easy3201.c \
    @srcdir@/board_easy3111.c \
    @srcdir@/device_duslic_xt.c
endif

if SYS_VINAX
common_SRC += \
    @srcdir@/board_easy80800.c
endif

if SYS_AMAZON_S
common_SRC += \
    @srcdir@/board_easy508xx.c

if M_TD_T38_FAX_TEST
nodist_tapidemo_SOURCES += \
    @srcdir@/t38_fax_test/td_t38_fax_test.c
endif

endif

if SYS_XWAY_XRX300

common_SRC += \
    @srcdir@/board_xwayXRX300.c

if M_TD_T38_FAX_TEST
nodist_tapidemo_SOURCES += \
    @srcdir@/t38_fax_test/td_t38_fax_test.c
endif

endif

if SYS_VR9
common_SRC += \
    @srcdir@/board_easy80910.c

if M_TD_T38_FAX_TEST
nodist_tapidemo_SOURCES += \
    @srcdir@/t38_fax_test/td_t38_fax_test.c
endif

endif

if DEV_VMMC
common_SRC += \
    @srcdir@/device_vmmc.c
endif

if SYS_EASY336
common_SRC += \
   @srcdir@/board_easy336.c \
   @srcdir@/device_svip.c
if M_WITH_VXT
common_SRC += \
   @srcdir@/board_xt16.c \
   @srcdir@/device_vxt.c
endif
if M_TD_T38_FAX_TEST
nodist_tapidemo_SOURCES += \
    @srcdir@/t38_fax_test/td_t38_fax_test.c

endif
endif

if SYS_XT16
common_SRC += \
   @srcdir@/board_xt16.c \
   @srcdir@/device_vxt.c
endif

EXTRA_DIST = \
    @top_srcdir@/tapidemo_version.h \
    @top_srcdir@/itm_version.h \
    @srcdir@/tapidemo.h \
    @srcdir@/common.h \
    @srcdir@/pcm.h \
    @srcdir@/qos.h \
    @srcdir@/cid.h \
    @srcdir@/conference.h \
    @srcdir@/abstract.h \
    @srcdir@/event_handling.h \
    @srcdir@/voip.h \
    @srcdir@/analog.h \
    @srcdir@/feature.h \
    @srcdir@/parse_cmd_arg.h \
    @srcdir@/td_t38.h \
    @srcdir@/state_trans.h \
    @srcdir@/itm/com_client.h \
    @srcdir@/itm/control_pc.h \
    @srcdir@/itm/dev_help.h \
    @srcdir@/lib_tapi_signal.h \
    @srcdir@/board_easy3332.c \
    @srcdir@/board_easy3332.h \
    @srcdir@/board_easy50712.c \
    @srcdir@/board_easy50712.h \
    @srcdir@/board_easy50510.c \
    @srcdir@/board_easy50510.h \
    @srcdir@/board_easy3201.c \
    @srcdir@/board_easy3201.h \
    @srcdir@/board_easy3111.c \
    @srcdir@/board_easy3111.h \
    @srcdir@/board_easy80800.c \
    @srcdir@/board_easy80800.h \
    @srcdir@/device_vinetic_cpe.c \
    @srcdir@/device_vinetic_cpe.h \
    @srcdir@/device_vmmc.c \
    @srcdir@/device_vmmc.h \
    @srcdir@/device_duslic_xt.c \
    @srcdir@/device_duslic_xt.h \
    @srcdir@/common_fxo.c \
    @srcdir@/common_fxo.h \
    @srcdir@/duslic_fxo.c \
    @srcdir@/duslic_fxo.h \
    @srcdir@/teridian_fxo.c \
    @srcdir@/teridian_fxo.h \
    @srcdir@/td_slic121_fxo.c \
    @srcdir@/td_slic121_fxo.h \
    @srcdir@/board_easy508xx.c \
    @srcdir@/board_easy508xx.h \
    @srcdir@/board_xwayXRX300.c \
    @srcdir@/board_xwayXRX300.h \
    @srcdir@/board_easy80910.c \
    @srcdir@/board_easy80910.h \
    @srcdir@/board_easy336.c \
    @srcdir@/board_easy336.h \
    @srcdir@/board_xt16.c \
    @srcdir@/board_xt16.h \
    @srcdir@/device_svip.c \
    @srcdir@/device_svip.h \
    @srcdir@/device_vxt.c \
    @srcdir@/device_vxt.h \
    @srcdir@/td_dect.c \
    @srcdir@/td_dect_stack.c \
    @srcdir@/td_dect.h \
    @srcdir@/td_dect_stack.h \
    @srcdir@/td_dect_config.c \
    @srcdir@/td_dect_config.h \
    @srcdir@/td_dect_cli.h \
    @srcdir@/td_osmap.h \
    @srcdir@/td_timer.c \
    @srcdir@/td_timer.h \
    @srcdir@/td_ifxos_map.h \
    @srcdir@/td_socket.h \
    @srcdir@/vxworks_tapidemo_cpe.wpj

INCLUDES = \
    -I@srcdir@ \
    -I@abs_top_builddir@ \
    -I@top_srcdir@ \
    -I@srcdir@/../bin \
    -I@KERNEL_INCL_PATH@ \
    -I@TAPI_INCL_PATH@ \
    -I@IFXOS_INCL_PATH@ \
    -I${IFXOS_LIBRARY_PATH}

if EVENT_LOGGER_DEBUG
INCLUDES += -I@EVENT_LOGGER_INCL_PATH@
endif

if SYS_EASY3332
INCLUDES += \
    -I@EASY3332_INCL_PATH@
endif

if DEV_VINETIC
INCLUDES += \
    -I@VINETIC_INCL_PATH@
endif

if DEV_VMMC
INCLUDES += \
    -I@VMMC_INCL_PATH@
if DEV_DXT
INCLUDES += \
    -I@DUSLICXT_INCL_PATH@
endif
else
if DEV_DXT
INCLUDES += \
    -I@DUSLICXT_INCL_PATH@
endif
if SYS_EASY3201
INCLUDES += \
    -I@EASY3201_INCL_PATH@
endif
endif

if SYS_EASY336
   INCLUDES += \
      -I@SVIP_INCL_PATH@ \
      -I@LIBSVIP_INCL_PATH@

if M_WITH_VXT
   INCLUDES += \
      -I@VXT_INCL_PATH@
endif
endif

if SYS_XT16
   INCLUDES += \
      -I@VXT_INCL_PATH@
endif

if SYS_USES_BOARD_DRV
   INCLUDES += \
      -I@BOARD_INCL_PATH@
endif

if SYS_USES_FXO
if M_FXO
common_SRC += \
    @srcdir@/common_fxo.c

if M_DUSLIC_FXO
common_SRC += \
    @srcdir@/duslic_fxo.c

INCLUDES += \
    -I@DUSLIC_INCL_PATH@
endif

if M_TERIDIAN_FXO
common_SRC += \
    @srcdir@/teridian_fxo.c

INCLUDES += \
    -I@TERIDIAN_INCL_PATH@
endif

if M_SLIC121_FXO
common_SRC += \
    @srcdir@/td_slic121_fxo.c
endif

endif
endif

if DEBUG
    AM_CFLAGS = -DLINUX  -O1 -g3
else
    AM_CFLAGS = -DLINUX  -O2
endif

# TAPIDEMO specific define - if some files from TAPIDEMO are compiled for other
# application then this flag is not set
AM_CFLAGS += -DTAPIDEMO_APP

AM_LDFLAGS = \
    -lpthread

tapidemo_LDADD = \
    ${IFXOS_LIBRARY_PATH}/libifxos.a

if M_TD_DECT
common_SRC += \
    @srcdir@/td_dect.c \
    @srcdir@/td_dect_stack.c \
    @srcdir@/td_dect_config.c

tapidemo_LDADD += \
    ${DECT_LIBRARY_PATH}/libdtk.a \
    ${DECT_LIBRARY_PATH}/libdectstack.a

INCLUDES += \
    -I${DECT_INCL_PATH}/ifx-dect

AM_CFLAGS += \
    ${DECT_CFLAGS}
endif

if M_TD_VOIP_COMMON
common_SRC += \
    @srcdir@/td_timer.c

tapidemo_LDADD += \
    ${VOIP_COMMON_LIBRARY_PATH}/libTimer.a \
    ${VOIP_COMMON_LIBRARY_PATH}/libDbg.a \
    ${VOIP_COMMON_LIBRARY_PATH}/libos.a

INCLUDES += \
    -I${VOIP_COMMON_INCL_PATH}/ifx-voip-common

AM_CFLAGS += \
    ${DECT_CFLAGS}
endif

if SYS_EASY336
AM_CFLAGS += \
    -Wall -DENABLE_TRACE -DAPP -DENABLE_LOG -DTAPI \
    -DVMMC_SVIP \
    -DIFXOS

AM_CFLAGS += -DSVIP \
   -DINCLUDE_SYSLOG

tapidemo_LDADD += \
    ${LIBSVIP_LIBRARY_PATH}/libsvip.a

endif

if SYS_XT16
AM_CFLAGS += \
    -Wall -DENABLE_TRACE -DAPP -DENABLE_LOG -DTAPI \
    -DVMMC_SVIP \
    -DIFXOS

endif

if TAPIDEMO_LIBRARY
libtapidemo_a_SOURCES = $(common_SRC)
else
tapidemo_SOURCES = $(common_SRC)
endif

lint:
	@flint @srcdir@/../std_lx.lnt $(INCLUDES)\
	-I@KERNEL_INCL_PATH@ \
	-I/home/${USER}/projects/buildroot/build_powerpc/staging_dir/include \
	-I/home/${USER}/projects/buildroot/build_powerpc/staging_dir/lib/gcc-lib/powerpc-linux-uclibc/3.3.4/include \
	$(common_SRC)

rsm:
	rsm -fa -n -s -Ta $(common_SRC) $(EXTRA_DIST)

install-exec-hook: $(bin_PROGRAMS)
	@for p in $(bin_PROGRAMS); do \
		$(STRIP) $(DESTDIR)$(bindir)/$$p; \
		echo "  $(STRIP) $(DESTDIR)$(bindir)/$$p"; \
	done
