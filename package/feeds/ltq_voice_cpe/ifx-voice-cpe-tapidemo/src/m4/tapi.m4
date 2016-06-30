dnl
dnl Option used to configure TAPI API version
dnl -------------------------------------------------
dnl TAPI_API_CHECK([DEFAULT-API], [ACTION-IF-TAPI4-ENABLED], [ACTION-IF-TAPI3-ENABLED])
dnl
dnl available values are:
dnl		--enable-tapi4
dnl		--enable-tapi3
dnl
AC_DEFUN([TAPI_VERSION_CHECK],
[
	AC_MSG_CHECKING(for TAPI API version)

	if test "${cached_tapi_version+set}" != set; then
		AC_ARG_ENABLE(tapi4,
			AS_HELP_STRING(
				[--enable-tapi4],
				[enable TAPI Version 4 interface (single device node)]
			),
			[__enable_tapi4=$enableval],
			[__enable_tapi4=ifelse([$1],[TAPI_VERSION4],[yes],[no])]
		)

		AC_ARG_ENABLE(tapi3,
			AS_HELP_STRING(
				[--enable-tapi3],
				[enable TAPI Version 3 interface (multiple device node)]
			),
			[__enable_tapi3=$enableval],
			[__enable_tapi3=ifelse([$1],[TAPI_VERSION3],[yes],[no])]
		)

		if test "$__enable_tapi4" = "yes" -o "$__enable_tapi4" = "enable"; then
			cached_tapi_version=TAPI_VERSION4
		fi

		if test "$__enable_tapi3" = "yes" -o "$__enable_tapi3" = "enable"; then
			if test "${cached_tapi_version+set}" != set; then
				cached_tapi_version=TAPI_VERSION3
			else
				AC_MSG_ERROR(
					[Only one of TAPI API can be enabled! Please select '--enable-tapi4' or '--enable-tapi3'.]
					)
			fi
		fi

		if test "${cached_tapi_version+set}" != set; then
			AC_MSG_ERROR(
				[The TAPI API interface should be defined! Please select '--enable-tapi4' or '--enable-tapi3'.]
				)
		fi

		AC_MSG_RESULT([$cached_tapi_version])

		echo "#define $cached_tapi_version" > $srcdir/include/drv_tapi_if_version.h

		unset __enable_tapi4 __enable_tapi3
	else
		AC_MSG_RESULT([$cached_tapi_version (cached)])
	fi

	if test "$cached_tapi_version" = TAPI_VERSION4; then
		ifelse([$2],,[:],[$2])
	else
		ifelse([$3],,[:],[$3])
	fi
])

dnl DRVTAPI_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_tapi
dnl specify --with-tapi-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([DRVTAPI_INCL_CHECK],
[
   dnl Check for existance of depricated option, terminate configuration on presence
   AC_ARG_ENABLE([tapiincl],,
   [
      AC_MSG_WARN([deprecated configure option --enable-tapiincl, use --with-tapi-incl instead])
   ])

	WITH_DRV_INCL_CHECK(
			[tapi],
			[TAPI driver],
			[drv_tapi drv_tapi-* voice_tapi_drv],
			[drv_tapi_io.h],
			[$1],
			[
				AC_SUBST([TAPI_INCL_PATH],[$cached_with_tapi_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl

dnl DRVTAPI_LIB_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for lib_tapi
dnl specify --with-tapi-lib
dnl If not supplied error results.
dnl
AC_DEFUN([DRVTAPI_LIB_CHECK],
[
   dnl determine absolute path to the srcdir
   if [[ -z "$abs_srcdir" ]]; then
      case $srcdir in
        .) abs_srcdir=$(pwd) ;;
        [[\\/]]* | ?:[[\\/]]* ) abs_srcdir=$srcdir ;;
        *) abs_srcdir=$(pwd)/$srcdir ;;
      esac
   fi

	__want_drvtapi_lib="no"
	__found_drvtapi_path="no"
	__found_drvtapi_lib="no"

   AC_MSG_CHECKING(for TAPI driver library)
   dnl Determine user choice for TAPI driver library
   AC_ARG_WITH([tapi-lib],
      AS_HELP_STRING(
         [--with-tapi-lib@<:@=DIR@:>@],
         [Include tapi @<:@default=<srcdir>/../drv_tapi/src@:>@. DIR is the path to the TAPI driver library.]),
		[
			__want_drvtapi_lib="yes"
			__with_drvtapi_lib=$withval
		],
		[__with_drvtapi_lib=ifelse([$1],,[$abs_srcdir/../drv_tapi/src],[$1])]
   )

	if test "x$__with_drvtapi_lib" != "xno"; then
		if test -f "$__with_drvtapi_lib/libdrvtapi.a"; then
			__found_drvtapi_path="yes"
			__found_drvtapi_lib="yes"
		elif test -f "$__with_drvtapi_lib/src/libdrvtapi.a"; then
			__found_drvtapi_path="yes"
			__found_drvtapi_lib="yes"
			__with_drvtapi_lib=${__with_drvtapi_lib}/src
		elif test -d "$__with_drvtapi_lib"; then
			__found_drvtapi_path="yes"
			__found_drvtapi_lib="no"
		fi
	fi

	AC_MSG_RESULT([$__with_drvtapi_lib])
	AC_SUBST([TAPI_LIBRARY_PATH],[$__with_drvtapi_lib])

	if test "x$__found_drvtapi_path" == "xyes"; then
		if test "x$__want_drvtapi_lib" == "xyes" -a "x$__found_drvtapi_lib" == "xno"; then
			AC_MSG_WARN([TAPI driver library path do not contain built library])
		fi

		ifelse([$2],,[:],[$2])
	else
		__msg="path not found, please specify correct value using '--with-tapi-lib'"

		if test "x$__want_drvtapi_lib" == "xyes"; then
			AC_MSG_ERROR([$__msg])
		fi

		AC_MSG_WARN([$__msg])

		unset __msg

		ifelse([$3],,[:],[$3])
	fi

   unset __found_drvtapi_path __found_drvtapi_lib __want_drvtapi_lib __with_drvtapi_lib

])dnl

dnl Option used to enable TAPI NLT/GR909
dnl -------------------------------------------------
dnl TAPI_NLT_CHECK([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-nlt
dnl		--disable-nlt (default)
dnl
AC_DEFUN([TAPI_NLT_CHECK],
[
	AC_MSG_CHECKING(for TAPI NLT/GR909 services)
	AC_ARG_ENABLE(nlt,
		AS_HELP_STRING(
			[--enable-nlt],
			[Enable TAPI Network Line Testing(NLT) services - including GR909]
		),
		[__enable_tapi_nlt=$enableval],
		[__enable_tapi_nlt=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_nlt" = "yes" -o "$__enable_tapi_nlt" = "enable"; then
		AC_DEFINE([TAPI_NLT],[1],[enable TAPI Network Line Testing services])
		AC_DEFINE([TAPI_GR909],[1],[enable TAPI GR909 support])
		AM_CONDITIONAL(WITH_NLT, true)
		AM_CONDITIONAL(WITH_GR909, true)
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AM_CONDITIONAL(WITH_NLT, false)
		AM_CONDITIONAL(WITH_GR909, false)
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_nlt
])

dnl Option used to enable TAPI LT/GR909
dnl -------------------------------------------------
dnl TAPI_LT_CHECK([1-DEFAULT-ACTION], [2-VARIABLE-TO-SET],
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-lt
dnl		--disable-lt (default)
dnl
AC_DEFUN([TAPI_LT_CHECK],
[
	AC_MSG_CHECKING(for TAPI line testing services)
	AC_ARG_ENABLE(lt,
		AS_HELP_STRING(
			[--enable-lt],
			[Enable line testing services including GR909]
		),
		[__enable_tapi_lt=$enableval],
		[__enable_tapi_lt=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_lt" = "yes" -o "$__enable_tapi_lt" = "enable"; then
		AC_MSG_RESULT([enabled])
      AC_DEFINE([$2],[1],[enable TAPI GR909 tests])

		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$4],,[:],[$4])
	fi

	unset __enable_tapi_lt
])

dnl Option used to enable TAPI LT GR909 and capacitance measurement for LL driver
dnl -------------------------------------------------
dnl TAPI_LL_LT_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME])
dnl
dnl available values are:
dnl		--enable-lt
dnl		--disable-lt (default)
dnl
AC_DEFUN([TAPI_LL_LT_CHECK],
[
	AC_ARG_ENABLE(lt,
		AS_HELP_STRING(
			[--enable-lt],
			[Enable line testing services including GR909]
		),
		[__enable_tapi_ll_lt=$enableval],
		[__enable_tapi_ll_lt=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_ll_lt" = "yes" -o "$__enable_tapi_ll_lt" = "enable"; then
      TAPI_GR909_CHECK([enable], $2)
      TAPI_CAP_MEASUREMENT_CHECK([enable], $2)
	else
      TAPI_GR909_CHECK([disable], $2)
      TAPI_CAP_MEASUREMENT_CHECK([disable], $2)
	fi

	unset __enable_tapi_ll_lt
])


dnl Option used to enable GR909
dnl -------------------------------------------------
dnl TAPI_GR909_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-gr909
dnl		--disable-gr909 (default)
dnl
AC_DEFUN([TAPI_GR909_CHECK],
[
	AC_MSG_CHECKING(for GR909 line testing services)
	AC_ARG_ENABLE(gr909,
		AS_HELP_STRING(
			[--enable-gr909],
			[Enable GR909 line testing services]
		),
		[__enable_gr9009=$enableval],
		[__enable_gr9009=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_gr9009" = "xyes" -o "x$__enable_gr9009" = "xenable"; then
		AC_MSG_RESULT([enabled])

		AC_DEFINE([$2_GR909_SUPPORT],[1],[enable GR909 line testing services])
      
		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])
      
		ifelse([$4],,[:],[$4])
	fi

	unset __enable_gr9009
])

dnl Option used to enable capacitance measurement
dnl -------------------------------------------------
dnl TAPI_CAP_MEASUREMENT_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-cap-measurement
dnl		--disable-cap-measurement (default)
dnl
AC_DEFUN([TAPI_CAP_MEASUREMENT_CHECK],
[
	AC_MSG_CHECKING(for capacitance measurement services)
	AC_ARG_ENABLE(cap-measurement,
		AS_HELP_STRING(
			[--enable-cap-measurement],
			[Enable capacitance measurement services]
		),
		[__enable_cap_meas=$enableval],
		[__enable_cap_meas=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_cap_meas" = "xyes" -o "x$__enable_cap_meas" = "xenable"; then
		AC_MSG_RESULT([enabled])

		AC_DEFINE([$2_CAPACITANCE_MEASUREMENT_SUPPORT],[1],
                [enable capacitance measurement services])
      
		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])
      
		ifelse([$4],,[:],[$4])
	fi

	unset __enable_cap_meas
])


dnl Option used to enable GPIO control of LL driver
dnl -------------------------------------------------
dnl TAPI_LL_GPIO_CTRL_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-gpio
dnl		--disable-gpio (default)
dnl
AC_DEFUN([TAPI_LL_GPIO_CTRL_CHECK],
[
	AC_MSG_CHECKING(for GPIO control support)
	AC_ARG_ENABLE(gpio,
		AS_HELP_STRING(
			[--enable-gpio],
			[Enable GPIO control]
		),
		[__enable_gpio=$enableval],
		[__enable_gpio=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_gpio" = "xyes" -o "x$__enable_gpio" = "xenable"; then
		AC_MSG_RESULT([enabled])

		AC_DEFINE([$2_GPIO_SUPPORT],[1],[enable GPIO control])
      
		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])
      
		ifelse([$4],,[:],[$4])
	fi

	unset __enable_gpio
])

dnl Option used to enable direct chip access for LL driver
dnl -------------------------------------------------
dnl TAPI_LL_DIRECT_CHIP_ACCESS_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-direct-chip-access
dnl		--disable-direct-chip-access (default)
dnl
AC_DEFUN([TAPI_LL_DIRECT_CHIP_ACCESS_CHECK],
[
	AC_MSG_CHECKING(for direct chip access support)
	AC_ARG_ENABLE(direct-chip-access,
		AS_HELP_STRING(
			[--enable-direct-chip-access],
			[Enable direct chip access support]
		),
		[__enable_direct_chip_access=$enableval],
		[__enable_direct_chip_access=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_direct_chip_access" = "xyes" -o "x$__enable_direct_chip_access" = "xenable"; then
		AC_MSG_RESULT([enabled])

		AC_DEFINE([$2_DIRECT_CHIP_ACCESS_SUPPORT],[1],[enable direct chip access support])
      
		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])
      
		ifelse([$4],,[:],[$4])
	fi

	unset __enable_direct_chip_access
])

dnl Option used to enable Kernel API for LL driver
dnl -------------------------------------------------
dnl TAPI_KERNEL_API_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-kernel-api
dnl		--disable-kernel-api (default)
dnl
AC_DEFUN([TAPI_KERNEL_API_CHECK],
[
	AC_MSG_CHECKING(for Kernel API support)
	AC_ARG_ENABLE(kernel-api,
		AS_HELP_STRING(
			[--enable-kernel-api],
			[Enable Kernel API]
		),
		[__enable_kernel_api=$enableval],
		[__enable_kernel_api=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_kernel_api" = "xyes" -o "x$__enable_kernel_api" = "xenable"; then
		AC_MSG_RESULT([enabled])

		AC_DEFINE([$2_KERNEL_API_SUPPORT],[1],[enable Kernel API])
      
		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])
      
		ifelse([$4],,[:],[$4])
	fi

	unset __enable_kernel_api
])

dnl Option used to enable tone generator feature for LL driver
dnl -------------------------------------------------
dnl TAPI_TONE_GENERATOR_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-tone-generator
dnl		--disable-tone-generator (default)
dnl
AC_DEFUN([TAPI_TONE_GENERATOR_CHECK],
[
	AC_MSG_CHECKING(for tone generator support)
	AC_ARG_ENABLE(tone-generator,
		AS_HELP_STRING(
			[--enable-tone-generator],
			[Enable tone generator]
		),
		[__enable_tone_generator=$enableval],
		[__enable_tone_generator=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_tone_generator" = "xyes" -o "x$__enable_tone_generator" = "xenable"; then
		AC_MSG_RESULT([enabled])

		AC_DEFINE([$2_TONE_GENERATOR_SUPPORT],[1],[enable tone generator feature])
      
		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])
      
		ifelse([$4],,[:],[$4])
	fi

	unset __enable_tone_generator
])

dnl Option used to enable TAPI voice
dnl -------------------------------------------------
dnl TAPI_VOICE_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-voice (default)
dnl		--disable-voice
dnl
AC_DEFUN([TAPI_VOICE_CHECK],
[
	AC_MSG_CHECKING(for TAPI Voice support)
	AC_ARG_ENABLE(voice,
		AS_HELP_STRING(
			[--enable-voice],
			[Enable Voice support]
		),
		[__enable_tapi_voice=$enableval],
		[__enable_tapi_voice=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_voice" = "yes" -o "$__enable_tapi_voice" = "enable"; then
		AC_DEFINE([TAPI_VOICE],[1],[enable Voice support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_voice
])

dnl Option used to enable TAPI DTMF
dnl -------------------------------------------------
dnl TAPI_DTMF_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-dtmf (default)
dnl		--disable-dtmf
dnl
AC_DEFUN([TAPI_DTMF_CHECK],
[
	AC_MSG_CHECKING(for TAPI DTMF support)
	AC_ARG_ENABLE(dtmf,
		AS_HELP_STRING(
			[--enable-dtmf],
			[Enable TAPI DTMF support]
		),
		[__enable_tapi_dtmf=$enableval],
		[__enable_tapi_dtmf=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_dtmf" = "yes" -o "$__enable_tapi_dtmf" = "enable"; then
		AC_DEFINE([TAPI_DTMF],[1],[enable TAPI DTMF support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_dtmf
])

dnl Option used to enable TAPI CID
dnl -------------------------------------------------
dnl TAPI_CID_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-cid (default)
dnl		--disable-cid
dnl
AC_DEFUN([TAPI_CID_CHECK],
[
	AC_MSG_CHECKING(for TAPI CID support)
	AC_ARG_ENABLE(cid,
		AS_HELP_STRING(
			[--enable-cid],
			[Enable TAPI CID support]
		),
		[__enable_tapi_cid=$enableval],
		[__enable_tapi_cid=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_cid" = "yes" -o "$__enable_tapi_cid" = "enable"; then
		AC_DEFINE([TAPI_CID],[1],[enable TAPI CID support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_cid
])

dnl Option used to enable TAPI T.38 FAX
dnl -------------------------------------------------
dnl TAPI_FAX_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-fax (default)
dnl		--disable-fax
dnl
AC_DEFUN([TAPI_FAX_CHECK],
[
	AC_MSG_CHECKING(for TAPI FAX support)
	AC_ARG_ENABLE(fax,
		AS_HELP_STRING(
			[--enable-fax],
			[Enable TAPI T.38 FAX support (T.38 is not supported by voice DSP)]
		),
		[__enable_tapi_fax=$enableval],
		[__enable_tapi_fax=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_fax" = "yes" -o "$__enable_tapi_fax" = "enable"; then
		AC_DEFINE([TAPI_FAX_T38],[1],[enable TAPI FAX support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_fax
])

dnl Option used to enable TAPI T.38 Stack
dnl -------------------------------------------------
dnl TAPI_FAXSTACK_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-t38 (default)
dnl		--disable-t38
dnl
AC_DEFUN([TAPI_FAXSTACK_CHECK],
[
	AC_MSG_CHECKING(for TAPI T.38 Stack support)
	AC_ARG_ENABLE(t38,
		AS_HELP_STRING(
			[--enable-t38],
			[Enable TAPI T.38 FAX support (T.38 is supported by voice DSP)]
		),
		[__enable_tapi_faxstack=$enableval],
		[__enable_tapi_faxstack=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_faxstack" = "yes" -o "$__enable_tapi_faxstack" = "enable"; then
		AC_DEFINE([TAPI_FAX_T38_FW],[1],[enable TAPI T.38 FAX stack support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_faxstack
])

dnl Option used to enable TAPI Announcements
dnl -------------------------------------------------
dnl TAPI_ANNOUNCEMENTS_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-announcements (default)
dnl		--disable-announcements
dnl
AC_DEFUN([TAPI_ANNOUNCEMENTS_CHECK],
[
	AC_MSG_CHECKING(for TAPI Announcements support)
	AC_ARG_ENABLE(announcements,
		AS_HELP_STRING(
			[--enable-announcements],
			[Enable TAPI Announcements support]
		),
		[__enable_tapi_announcements=$enableval],
		[__enable_tapi_announcements=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_announcements" = "yes" -o "$__enable_tapi_announcements" = "enable"; then
		AC_DEFINE([TAPI_ANNOUNCEMENTS],[1],[enable TAPI Announcements support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_announcements
])
dnl
dnl Option used to enable TAPI Tone Level Peak Detector
dnl -------------------------------------------------
dnl TAPI_PEAKD_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-peakd (default)
dnl		--disable-peakd
dnl
AC_DEFUN([TAPI_PEAKD_CHECK],
[
	AC_MSG_CHECKING(for TAPI Tone Level Peak Detector support)
	AC_ARG_ENABLE(peakd,
		AS_HELP_STRING(
			[--enable-peakd],
			[Enable TAPI Tone Level Peak Detector support]
		),
		[__enable_tapi_peakd=$enableval],
		[__enable_tapi_peakd=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_peakd" = "yes" -o "$__enable_tapi_peakd" = "enable"; then
		AC_DEFINE([TAPI_PEAKD],[1],[enable TAPI Tone Level Peak Detector support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_peakd
])

dnl
dnl Option used to enable TAPI MF R2 Detector
dnl -------------------------------------------------
dnl TAPI_MF_R2_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-mfr2 (default)
dnl		--disable-mfr2
dnl
AC_DEFUN([TAPI_MF_R2_CHECK],
[
	AC_MSG_CHECKING(for TAPI MF R2 tone detector support)
	AC_ARG_ENABLE(mfr2,
		AS_HELP_STRING(
			[--enable-mfr2],
			[Enable TAPI MF R2 tone detector support]
		),
		[__enable_tapi_mfr2=$enableval],
		[__enable_tapi_mfr2=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_mfr2" = "yes" -o "$__enable_tapi_mfr2" = "enable"; then
		AC_DEFINE([TAPI_MF_R2],[1],[enable TAPI MF R2 tone detector support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_mfr2
])

dnl
dnl Option used to enable TAPI Kernel Packet Interface (KPI)
dnl -------------------------------------------------
dnl TAPI_KPI_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-kpi
dnl		--disable-kpi (default)
dnl
AC_DEFUN([TAPI_KPI_CHECK],
[
	AC_MSG_CHECKING(for Kernel Packet Interface)

	if test "${cached_enable_tapi_kpi+set}" != set; then
		AC_ARG_ENABLE(kpi,
			AS_HELP_STRING(
				[--enable-kpi],
				[enable KPI - Kernel Packet Interface]
			),
			[__enable_tapi_kpi=$enableval],
			[__enable_tapi_kpi=ifelse([$1],,[no],[$1])]
		)

		if test "$__enable_tapi_kpi" = "yes" -o "$__enable_tapi_kpi" = "enable"; then
			cached_enable_tapi_kpi=enabled
			AC_DEFINE([KPI_SUPPORT],[1],[enable KPI support])
			AM_CONDITIONAL(KPI_SUPPORT, true)
			AC_MSG_RESULT([enabled])

		else
			cached_enable_tapi_kpi=disabled
			AM_CONDITIONAL(KPI_SUPPORT, false)
			AC_MSG_RESULT([disabled])
		fi

		unset __enable_tapi_kpi
	else
		AC_MSG_RESULT([$cached_enable_tapi_kpi (cached)])
	fi

	if test "$cached_enable_tapi_kpi" = enabled; then
		ifelse([$2],,[:],[$2])
	else
		ifelse([$3],,[:],[$3])
	fi
])

dnl
dnl Option used to enable KPI tasklet mode
dnl -------------------------------------------------
dnl TAPI_KPI_TASKLET_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-kpi-tasklet (default)
dnl		--disable-kpi-tasklet
dnl
AC_DEFUN([TAPI_KPI_TASKLET_CHECK],
[
	AC_MSG_CHECKING(for KPI tasklet mode)
	AC_ARG_ENABLE(kpi-tasklet,
		AS_HELP_STRING(
         [--enable-kpi-tasklet],
         [enable KPI tasklet mode - linux specific]
		),
		[__enable_tapi_kpi_tasklet=$enableval],
		[__enable_tapi_kpi_tasklet=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_kpi_tasklet" = "yes" -o "$__enable_tapi_kpi_tasklet" = "enable"; then
                AC_DEFINE([KPI_TASKLET],[1],[enable KPI tasklet mode])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_kpi_tasklet
])

dnl
dnl Option used to enable Quality of Service and UDP redirection (QoS)
dnl -------------------------------------------------
dnl TAPI_QOS_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-qos
dnl		--disable-qos (default)
dnl
AC_DEFUN([TAPI_QOS_CHECK],
[
	AC_MSG_CHECKING(for Quality of Service and UDP redirection)
	AC_ARG_ENABLE(qos,
		AS_HELP_STRING(
         [--enable-qos],
         [enable QoS - quality of service and UDP redirection]
		),
		[__enable_tapi_qos=$enableval],
		[__enable_tapi_qos=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_qos" = "yes" -o "$__enable_tapi_qos" = "enable"; then
      AC_DEFINE([QOS_SUPPORT],[1],[enable QOS support])
      AM_CONDITIONAL(QOS_SUPPORT, true)
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])

		dnl check for KPI support
		TAPI_KPI_CHECK([enable],,[AC_MSG_ERROR([KPI feature required for QOS])])
	else
      AM_CONDITIONAL(QOS_SUPPORT, false)
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_qos
])

dnl
dnl Option used to enable HDLC feature
dnl -------------------------------------------------
dnl TAPI_HDLC_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-hdlc
dnl		--disable-hdlc (default)
dnl
AC_DEFUN([TAPI_HDLC_CHECK],
[
	AC_MSG_CHECKING(for HDLC feature)
	AC_ARG_ENABLE(hdlc,
		AS_HELP_STRING(
         [--enable-hdlc],
         [enable HDLC feature]
		),
		[__enable_tapi_hdlc=$enableval],
		[__enable_tapi_hdlc=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_hdlc" = "yes" -o "$__enable_tapi_hdlc" = "enable"; then
		AC_DEFINE([TAPI_HDLC],[1],[enabled])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])

		dnl check for KPI support
		TAPI_KPI_CHECK([enable],,[AC_MSG_ERROR([KPI feature required for HDLC])])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_hdlc
])

dnl
dnl Option used to enable HDLC inter frame idle pattern
dnl -------------------------------------------------
dnl TAPI_HDLC_IDLE_PATTERN_CHECK([HDLC-IS-SET])
dnl
dnl available values are:
dnl		--enable-hdlc
dnl		--disable-hdlc (default)
dnl
AC_DEFUN([TAPI_HDLC_IDLE_PATTERN_CHECK],
[
   if test "$1" = "yes"; then
      AC_MSG_CHECKING(for HDLC inter frame idle pattern)
   fi
   AC_ARG_ENABLE(hdlc-idle-patt,
      AS_HELP_STRING(
         [--enable-hdlc-idle-patt=val],
         [enable HDLC inter frame idle pattern, val=0xFF|0x7E, default is 0x7E]
      ),
      [
         if test "$1" = "yes"; then
            if test -n $enableval; then
               if test $enableval = "0xFF" -o $enableval = "0xff"; then
                  AC_MSG_RESULT([0xFF (IDLE)])
                  AC_DEFINE([HDLC_IDLE_PATTERN],[1],[enable HDLC inter frame idle pattern 0xFF])
               elif test $enableval = "0x7E" -o $enableval = "0x7e"; then
                  AC_MSG_RESULT([0x7E (FLAGS)])
               else
                  AC_MSG_ERROR([cannot enable HDLC idle pattern, valid values are 0xFF or 0x7E])
               fi
            fi
         else
            AC_MSG_ERROR([cannot enable HDLC idle pattern, please enable HDLC feature with --enable-hdlc])
         fi
      ],
      [
         if test "$1" = "yes"; then
            AC_MSG_RESULT(0x7E (FLAGS) - default)
         fi
      ]
   )
])

dnl
dnl Option used to enable TAPI Hook state machine
dnl -------------------------------------------------
dnl TAPI_HSM_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-hsm (default)
dnl		--disable-hsm
dnl
AC_DEFUN([TAPI_HSM_CHECK],
[
	AC_MSG_CHECKING(for TAPI Hook state machine)
	AC_ARG_ENABLE(hsm,
		AS_HELP_STRING(
         [--enable-hsm],
         [enable TAPI Hook state machine]
		),
		[__enable_tapi_hsm=$enableval],
		[__enable_tapi_hsm=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_hsm" = "yes" -o "$__enable_tapi_hsm" = "enable"; then
		AC_DEFINE([TAPI_HOOKSTATE],[1],[enable TAPI Hook state machine])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_hsm
])


dnl
dnl Option used to enable TAPI Metering feature
dnl -------------------------------------------------
dnl TAPI_METERING_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-metering
dnl		--disable-metering (default)
dnl
AC_DEFUN([TAPI_METERING_CHECK],
[
	AC_MSG_CHECKING(for TAPI METERING support)
	AC_ARG_ENABLE(metering,
		AS_HELP_STRING(
         [--enable-metering],
         [enable TAPI METERING support]
		),
		[__enable_tapi_metering=$enableval],
		[__enable_tapi_metering=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_metering" = "yes" -o "$__enable_tapi_metering" = "enable"; then
		AC_DEFINE([TAPI_METERING],[1],[enable TAPI METERING support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_metering
])

dnl
dnl Option used to enable read/write packet interface
dnl -------------------------------------------------
dnl TAPI_PACKET_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-packet (default)
dnl		--disable-packet
dnl
AC_DEFUN([TAPI_PACKET_CHECK],
[
	AC_MSG_CHECKING(for read/write packet interface)
	AC_ARG_ENABLE(packet,
		AS_HELP_STRING(
         [--enable-packet],
			[enable read/write packet interface]
		),
		[__enable_tapi_packet=$enableval],
		[__enable_tapi_packet=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_packet" = "yes" -o "$__enable_tapi_packet" = "enable"; then
		AM_CONDITIONAL(PACKET_SUPPORT, true)
		AC_DEFINE([TAPI_PACKET],[1],[enabled read/write packet interface])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AM_CONDITIONAL(PACKET_SUPPORT, false)
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_packet
])

dnl
dnl Option used to enable TAPI Analog Line Continuous Measurement
dnl -------------------------------------------------
dnl TAPI_CONT_MEAS_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-cont-measurement
dnl		--disable-cont-measurement (default)
dnl
AC_DEFUN([TAPI_CONT_MEAS_CHECK],
[
	AC_MSG_CHECKING(for Analog Line Continuous Measurement)
	AC_ARG_ENABLE(cont-measurement,
		AS_HELP_STRING(
         [--enable-cont-measurement],
         [enable TAPI Analog Line Continuous Measurement]
		),
		[__enable_tapi_cont_meas=$enableval],
		[__enable_tapi_cont_meas=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_cont_meas" = "yes" -o "$__enable_tapi_cont_meas" = "enable"; then
		AC_DEFINE([TAPI_CONT_MEASUREMENT],[1],[enable TAPI Analog Line Continuous Measurement])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_cont_meas
])

dnl
dnl Option used to enable TAPI FXS Phone Detection support
dnl -------------------------------------------------
dnl TAPI_PHONE_DET_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-phone-detection
dnl		--disable-phone-detection (default)
dnl
AC_DEFUN([TAPI_PHONE_DET_CHECK],
[
	AC_MSG_CHECKING(for FXS Phone Detection support)
	AC_ARG_ENABLE(phone-detection,
		AS_HELP_STRING(
         [--enable-phone-detection],
         [enable TAPI FXS Phone Detection support]
		),
		[__enable_tapi_phone_det=$enableval],
		[__enable_tapi_phone_det=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_phone_det" = "yes" -o "$__enable_tapi_phone_det" = "enable"; then
		AC_DEFINE([TAPI_PHONE_DETECTION],[1],[enable FXS Phone Detection support])
		AM_CONDITIONAL(TAPI_PHONE_DETECTION, true)
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])
		AM_CONDITIONAL(TAPI_PHONE_DETECTION, false)

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_phone_det
])

dnl
dnl Option used to enable Power Management Unit support
dnl -------------------------------------------------------
dnl TAPI_PMU_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-pmu
dnl		--disable-pmu (default)
dnl
AC_DEFUN([TAPI_PMU_CHECK],
[
	AC_MSG_CHECKING(for Power Management Unit support)
	AC_ARG_ENABLE(pmu,
		AS_HELP_STRING(
         [--enable-pmu],
         [enable Power Management Unit support]
		),
		[__enable_tapi_pmu=$enableval],
		[__enable_tapi_pmu=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_pmu" = "yes" -o "$__enable_tapi_pmu" = "enable"; then
      AC_DEFINE([PMU_SUPPORTED],[1],[enable PMU support])
      AM_CONDITIONAL(PMU_SUPPORT, true)
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])
		AM_CONDITIONAL(PMU_SUPPORT, false)

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_pmu
])

dnl
dnl Option used to enable Power Management Control support
dnl -------------------------------------------------------
dnl TAPI_PMC_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-pmc
dnl		--disable-pmc (default)
dnl
AC_DEFUN([TAPI_PMC_CHECK],
[
	AC_MSG_CHECKING(for Power Management Control support)
	AC_ARG_ENABLE(pmc,
		AS_HELP_STRING(
         [--enable-pmc],
         [enable Power Management Control support]
		),
		[__enable_tapi_pmc=$enableval],
		[__enable_tapi_pmc=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_pmc" = "yes" -o "$__enable_tapi_pmc" = "enable"; then
		AC_DEFINE([TAPI_PMC],[1],[enable Power Management Control support])
		AM_CONDITIONAL(TAPI_PMC, true)
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])
		AM_CONDITIONAL(TAPI_PMC, false)

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_pmc
])

dnl
dnl Option used to enable DECT channel support
dnl -------------------------------------------------
dnl TAPI_DECT_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-dect
dnl		--disable-dect (default)
dnl
AC_DEFUN([TAPI_DECT_CHECK],
[
	AC_MSG_CHECKING(for DECT channel support)
	AC_ARG_ENABLE(dect,
		AS_HELP_STRING(
         [--enable-dect],
         [enable DECT channel support]
		),
		[__enable_tapi_dect=$enableval],
		[__enable_tapi_dect=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_dect" = "yes" -o "$__enable_tapi_dect" = "enable"; then
		AM_CONDITIONAL(DECT_SUPPORT, true)
		AC_DEFINE([DECT_SUPPORT],[1],[enable DECT channel support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
      AM_CONDITIONAL(DECT_SUPPORT, false)
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_dect
])

dnl
dnl Option used to disable DECT nibble swap for COSIC modem
dnl -------------------------------------------------
dnl TAPI_DECT_NIBBLE_SWAP_CHECK(
dnl [DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-dect-nibble-swap (default)
dnl		--disable-dect-nibble-swap
dnl
AC_DEFUN([TAPI_DECT_NIBBLE_SWAP_CHECK],
[
	AC_MSG_CHECKING(for DECT nibble swap for COSIC modem)
	AC_ARG_ENABLE(dect-nibble-swap,
		AS_HELP_STRING(
         [--enable-dect-nibble-swap],
         [disable DECT nibble swap for COSIC modem (for backward compatibiliy only)]
		),
		[__enable_tapi_dect_nibble_swap=$enableval],
		[__enable_tapi_dect_nibble_swap=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_dect_nibble_swap" = "yes" -o "$__enable_tapi_dect_nibble_swap" = "enable"; then
      AC_DEFINE([VMMC_DECT_NIBBLE_SWAP], [1], [enable DECT nibble swap for COSIC modem (default is 1)])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
      AC_DEFINE([VMMC_DECT_NIBBLE_SWAP], [0], [disable DECT nibble swap for COSIC modem (default is 1)])
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_dect_nibble_swap
])

dnl
dnl Option used to enable PCM channel support
dnl -------------------------------------------------
dnl TAPI_PCM_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-pcm (default)
dnl		--disable-pcm
dnl
AC_DEFUN([TAPI_PCM_CHECK],
[
	AC_MSG_CHECKING(for PCM support)

	if test "${cached_enable_tapi_pcm+set}" != set; then
		AC_ARG_ENABLE(pcm,
			AS_HELP_STRING(
				[--enable-pcm],
				[enable PCM channel support]
			),
			[__enable_tapi_pcm=$enableval],
			[__enable_tapi_pcm=ifelse([$1],,[yes],[$1])]
		)

		if test "$__enable_tapi_pcm" = "yes" -o "$__enable_tapi_pcm" = "enable"; then
			cached_enable_tapi_pcm=enabled
			AC_DEFINE([PCM_SUPPORT],[1],[enable PCM channel support])
			AM_CONDITIONAL(PCM_SUPPORT, true)
			AC_MSG_RESULT([enabled])

		else
			cached_enable_tapi_pcm=disabled
			AM_CONDITIONAL(PCM_SUPPORT, false)
			AC_MSG_RESULT([disabled])
		fi

		unset __enable_tapi_pcm
	else
		AC_MSG_RESULT([$cached_enable_tapi_pcm (cached)])
	fi

	if test "$cached_enable_tapi_pcm" = enabled; then
		ifelse([$2],,[:],[$2])
	else
		ifelse([$3],,[:],[$3])
	fi
])

dnl
dnl Option used to enable TAPI packet path statistics support
dnl -------------------------------------------------
dnl TAPI_PKG_STAT_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-statistics
dnl		--disable-statistics (default)
dnl
AC_DEFUN([TAPI_PKG_STAT_CHECK],
[
	AC_MSG_CHECKING(for TAPI packet path statistics)
	AC_ARG_ENABLE(statistics,
		AS_HELP_STRING(
         [--enable-statistics],
         [enable TAPI packet path statistics support]
		),
		[__enable_tapi_pkg_stat=$enableval],
		[__enable_tapi_pkg_stat=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_pkg_stat" = "yes" -o "$__enable_tapi_pkg_stat" = "enable"; then
		AC_DEFINE([TAPI_STATISTICS],[1],[enable packet path statistics support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_pkg_stat
])

dnl
dnl Option used to enable owner ID usage on packet interface
dnl DEBUG feature to tag buffers on their way through the system to detect leakages
dnl -------------------------------------------------
dnl TAPI_PKG_OWNER_ID_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-packet-owner-id
dnl		--disable-packet-owner-id (default)
dnl
AC_DEFUN([TAPI_PKG_OWNER_ID_CHECK],
[
	AC_MSG_CHECKING(for owner ID on packet interface)
	AC_ARG_ENABLE(packet-owner-id,
		AS_HELP_STRING(
         [--enable-packet-owner-id],
         [enable owner ID usage on packet interface]
			[DEBUG feature to tag buffers on their way through the system to detect leakages]
		),
		[__enable_tapi_pkg_owner_id=$enableval],
		[__enable_tapi_pkg_owner_id=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_pkg_owner_id" = "yes" -o "$__enable_tapi_pkg_owner_id" = "enable"; then
		AC_DEFINE([TAPI_PACKET_OWNID],[1],[enabled owner ID for packet interface])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_pkg_owner_id
])

dnl
dnl Option used to enable hotplug events
dnl -------------------------------------------------
dnl TAPI_HOTPLUG_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-hotplug
dnl		--disable-hotplug (default)
dnl
AC_DEFUN([TAPI_HOTPLUG_CHECK],
[
	AC_MSG_CHECKING(for hotplug events support)
	AC_ARG_ENABLE(hotplug,
		AS_HELP_STRING(
         [--enable-hotplug],
         [enable hotplug events support]
		),
		[__enable_tapi_hotplug=$enableval],
		[__enable_tapi_hotplug=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_hotplug" = "yes" -o "$__enable_tapi_hotplug" = "enable"; then
		AC_DEFINE([ENABLE_HOTPLUG],[1],[enable hotplug events in general])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_dect
])

dnl Option used to enable SRTP support
dnl -------------------------------------------------
dnl TAPI_SRTP_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-srtp (default)
dnl		--disable-srtp
dnl
AC_DEFUN([TAPI_SRTP_CHECK],
[
	AC_MSG_CHECKING(for TAPI SRTP support)
	AC_ARG_ENABLE(srtp,
		AS_HELP_STRING(
			[--enable-srtp],
			[Enable SRTP support]
		),
		[__enable_tapi_srtp=$enableval],
		[__enable_tapi_srtp=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_srtp" = "yes" -o "$__enable_tapi_srtp" = "enable"; then
		AC_DEFINE([TAPI_SRTP],[1],[enable TAPI SRTP support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_srtp
])

