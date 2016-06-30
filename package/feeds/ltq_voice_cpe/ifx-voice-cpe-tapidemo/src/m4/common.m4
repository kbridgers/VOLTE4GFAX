dnl Option used to select an compiler warnings
dnl -------------------------------------------------
dnl WARNINGS_CHECK ([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl available values are:
dnl		--enable-warnings
dnl		--disable-warnings (default)
dnl
AC_DEFUN([WARNINGS_CHECK],
[
	AC_MSG_CHECKING(for warnings)
	AC_ARG_ENABLE(warnings,
		AS_HELP_STRING(
			[--enable-warnings],
			[enable compiler warnings (GCC only)]
		),
		[
			if test $enableval = 'yes'; then
				AM_CONDITIONAL(WARNINGS, true)
				CFLAGS="$CFLAGS -Wall"

				ifelse([$1],,:,[$1])

				AC_MSG_RESULT([enabled])
			else
				AM_CONDITIONAL(WARNINGS, false)

				ifelse([$2],,:,[$2])

				AC_MSG_RESULT([disabled])
			fi
		],
		[
			AM_CONDITIONAL(WARNINGS, false)

			ifelse([$2],,:,[$2])

			AC_MSG_RESULT([disabled (default), enable with --enable-warnings])
		]
	)
])

dnl Option used to enable warnings as error
dnl -------------------------------------------------
dnl WERROR_CHECK ([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-werror
dnl		--disable-werror (default)
dnl
AC_DEFUN([WERROR_CHECK],
[
	AC_MSG_CHECKING(for warnings as errors)
	AC_ARG_ENABLE(werror,
		AS_HELP_STRING(
			[--enable-werror],
			[enable warnings as error]
		),
		[
			if test $enableval = 'yes'; then
				AM_CONDITIONAL(WERROR, true)
				CFLAGS="$CFLAGS -Werror"

				ifelse([$1],,:,[$1])

				AC_MSG_RESULT([enabled])
			else
				AM_CONDITIONAL(WERROR, false)

				ifelse([$2],,:,[$2])

				AC_MSG_RESULT([disabled])
			fi
		],
		[
			AM_CONDITIONAL(WERROR, false)

			ifelse([$2],,:,[$2])

			AC_MSG_RESULT([disabled (default), enable with --enable-werror])
		]
	)
])

dnl Option used to enable debugging messages
dnl -------------------------------------------------
dnl DEBUG_CHECK ([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-debug
dnl		--disable-debug (default)
dnl
AC_DEFUN([DEBUG_CHECK],
[
	AC_MSG_CHECKING(for debugging messages)
	AC_ARG_ENABLE(debug,
		AS_HELP_STRING(
			[--enable-debug],
			[Build in debugging symbols and enable debugging messages]
		),
		[
			if test $enableval = 'yes'; then
				AC_DEFINE([DEBUG],[1],[Build in debugging symbols and enable debugging messages])
				AM_CONDITIONAL(DEBUG, true)
				CFLAGS=`echo $CFLAGS | sed -e 's#\-g[0-9]*##g'`
				CFLAGS=`echo $CFLAGS | sed -e 's#\-O[0-9s]*##g'`
				CFLAGS="$CFLAGS -O1 -g3"

				ifelse([$1],,:,[$1])

				AC_MSG_RESULT([enabled])
			else
				AM_CONDITIONAL(DEBUG, false)

				ifelse([$2],,:,[$2])

				AC_MSG_RESULT([disabled])
			fi
		],
		[
			AM_CONDITIONAL(DEBUG, false)

			ifelse([$2],,:,[$2])

			AC_MSG_RESULT([disabled (default), enable with --enable-debug])
		]
	)
])

dnl Option used to enable runtime traces
dnl -------------------------------------------------
dnl TRACE_CHECK ([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-trace
dnl		--disable-trace (default)
dnl
AC_DEFUN([TRACE_CHECK],
[
	AC_MSG_CHECKING(for runtime traces)
	AC_ARG_ENABLE(trace,
		AS_HELP_STRING(
			[--enable-trace],
			[enable runtime traces]
		),
		[
			if test $enableval = 'yes'; then
				AM_CONDITIONAL(ENABLE_TRACE, true)
				AC_DEFINE([ENABLE_TRACE],[1],[enable trace outputs in general])
				AC_DEFINE([ENABLE_LOG],[1],[enable log (errors) outputs in general])

				ifelse([$1],,:,[$1])

				AC_MSG_RESULT([enabled])
			else
				AM_CONDITIONAL(ENABLE_TRACE, false)

				ifelse([$2],,:,[$2])

				AC_MSG_RESULT([disabled])
			fi
		],
		[
			AM_CONDITIONAL(ENABLE_TRACE, false)

			ifelse([$2],,:,[$2])

			AC_MSG_RESULT([disabled (default), enable with --enable-trace])
		]
	)
])


dnl Option used to set additional (device specific) CFLAGS
dnl -------------------------------------------------
dnl
AC_DEFUN([WITH_CFLAGS_CHECK],
[
	AC_MSG_CHECKING(for additional CFLAGS)
	AC_ARG_WITH(cflags,
		AS_HELP_STRING(
			[--with-cflags=val],
			[pass additional (device specific) CFLAGS, not required for Linux 2.6]
		),
		[
			if test "$withval" = yes; then
				AC_MSG_ERROR([Please provide a value for the maximum devices]);
			fi
			CFLAGS="$CFLAGS $withval"
			AC_MSG_RESULT([$withval])
		],
		[
			AC_MSG_RESULT([not set (default), set with --with-cflags])
		]
	)
])

dnl Option used to enable counting of events
dnl -------------------------------------------------
dnl EVENT_COUNTER_CHECK ([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-event-counter
dnl		--disable-event-counter (default)
dnl
AC_DEFUN([EVENT_COUNTER_CHECK],
[
	AC_MSG_CHECKING(for event counter)
	AC_ARG_ENABLE(event-counter,
		AS_HELP_STRING(
			[--enable-event-counter],
			[Enable counting of events]
		),
		[
			if test $enableval = 'yes'; then
				AC_DEFINE([EVENT_COUNTER],[1],[Enable counting of events])

				ifelse([$1],,:,[$1])

				AC_MSG_RESULT([enabled])
			else
				ifelse([$2],,:,[$2])

				AC_MSG_RESULT([disabled])
			fi
		],
		[
			ifelse([$2],,:,[$2])

			AC_MSG_RESULT([disabled (default), enable with --enable-event-counter])
		]
	)
])
dnl
dnl Option used to enable/disable interrupts
dnl -------------------------------------------------
dnl INTERRUPTS_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-interrupts (default)
dnl		--disable-interrupts
dnl
AC_DEFUN([INTERRUPTS_CHECK],
[
	AC_MSG_CHECKING(for interrupts)
	AC_ARG_ENABLE(interrupts,
		AS_HELP_STRING(
			[--enable-interrupts],
			[Enable/disable interrupt support]
		),
		[__enable_interrupts=$enableval],
		[__enable_interrupts=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_interrupts" = "yes" -o "$__enable_interrupts" = "enable"; then
		ifelse([$2],,:,[$2])

		AC_MSG_RESULT([enabled])
	else
		ifelse([$3],,:,[$3])

		AC_MSG_RESULT([disabled])
	fi

	unset __enable_interrupts
])
dnl
dnl Searching for the driver path
dnl -------------------------------------------------
dnl DRV_PATH_CHECK([VAR-NAME], [DRV-NAMES], [BASE-DIRS])
dnl
dnl VAR-NAME 	- [out] variable name to store result
dnl DRV-NAMES	- list of driver names
dnl BASE-DIRS 	- list of base directories
dnl
AC_DEFUN([DRV_PATH_CHECK],
[
	__pwd=$(pwd)

	dnl determine absolute path to the srcdir
	if [[ -z "$__abs_srcdir" ]]; then
		cd "$srcdir"
		__abs_srcdir=$(pwd)
		cd "$__pwd"
	fi

	if test "${cached_found_base_drv_path+set}" != set; then
		__want_base_drv_path="no"
		__found_base_drv_path="no"

		AC_ARG_WITH([drv-incl],
			AS_HELP_STRING(
				[--with-drv-incl@<:@=DIR@:>@],
				[Path to the base directory for driver search includes.]
				[@<:@default=<abs_srcdir>/..@:>@.]
				),
			 [
				__with_base_drv_path=$withval
				__want_base_drv_path="yes"
			 ],
			[__with_base_drv_path="$__abs_srcdir/.."]
		)

		if test -d "$__with_base_drv_path"; then
			__found_base_drv_path="yes"
		else
			if test "x$__want_base_drv_path" == "xyes"; then
				AC_MSG_RESULT([$__with_base_drv_path (no)(intermediate)])
				AC_MSG_ERROR([incorrect basedir passed, please specify correct value using '--with-drv-incl'])
			fi
		fi

		cached_found_base_drv_path=$__found_base_drv_path;
		cached_with_base_drv_path=$__with_base_drv_path;

		unset __found_base_drv_path __with_base_drv_path __want_base_drv_path
	fi

	for __base in $cached_with_base_drv_path $3 $__abs_srcdir; do
		cd "$__pwd"
		cd "$__base" &> /dev/null || continue
		for __name in $2; do
			cd "./$__name" &> /dev/null || continue
			__drv_path=$(pwd)
			break 3
		done
	done

	cd "$__pwd"
	unset __pwd

	if [[ "x$__drv_path" != "x" ]]; then
		$1=$__drv_path
	else
		$1=$__abs_srcdir
	fi

	unset __base __rel __name __drv_path
])
dnl
dnl Searching for correct path of header file
dnl -------------------------------------------------
dnl DRV_HEADER_PATH_CHECK([VAR-NAME], [HEARED-NAME], [SUB-DIRS],[ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl
dnl VAR-NAME 		- [in/out] variable name with base directory
dnl HEARED-NAME	- header file name
dnl SUB-DIRS		- list of subdirectory names (default: . include src)
dnl
AC_DEFUN([DRV_HEADER_PATH_CHECK],
[
	__pwd=$(pwd)

	for __rel in ifelse([$3],,. include src,[$3]); do
		cd "$__pwd"
		cd "$$1/$__rel" &> /dev/null || continue
		if test -f "$2"; then
			__hdr_path=$(pwd)
			break
		fi
	done

	cd "$__pwd"

	unset __pwd __rel

	if [[ "x$__hdr_path" != "x" ]]; then
		$1=$__hdr_path

		ifelse([$4],,:,[$4])
	else
		ifelse([$5],,:,[$5])
	fi

	unset __hdr_path
])
dnl
dnl WITH_DRV_INCL_CHECK(
dnl 	[WITH-INCL], [DRV-NAME], [DRV-DIRS], [DRV-HEADER],
dnl 	[DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED]
dnl ----------------------------------------------------------
dnl
dnl Checks for [DRV-NAME]
dnl specify --with-[WITH-INCL]-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([WITH_DRV_INCL_CHECK],
[
	AC_MSG_CHECKING(for $2 includes)
	if test "${cached_found_$1_incl+set}" != set; then
		__want_$1_incl="no"
		__found_$1_incl="no"

		DRV_PATH_CHECK([__$1_default_incl],[$3])

		AC_ARG_WITH([$1-incl],
			AS_HELP_STRING(
				[--with-$1-incl@<:@=DIR@:>@],
				[Path to $2 includes.]
				[@<:@default=<basedir>/<drv_name>/include@:>@; ]
				[<drv_name> are list of @<:@$3@:>@; ]
				[<basedir> are configurable with '--with-drv-incl']
				),
			 [
				__with_$1_incl=$withval
				__want_$1_incl="yes"
			 ],
			[__with_$1_incl=ifelse([$5],,[$__$1_default_incl],[$5])]
		)

		DRV_HEADER_PATH_CHECK([__with_$1_incl], [$4],,
			[__found_$1_incl="yes"])

		AC_MSG_RESULT([$__with_$1_incl ($__found_$1_incl)])
		cached_found_$1_incl=$__found_$1_incl;
		cached_want_$1_incl=$__want_$1_incl;
		cached_with_$1_incl=$__with_$1_incl;

		unset __found_$1_incl __with_$1_incl __want_$1_incl
	else
		AC_MSG_RESULT([$cached_with_$1_incl ($cached_found_$1_incl) (cached)])
	fi

	if test "x$cached_found_$1_incl" == "xyes"; then
		ifelse([$6],,[:],[$6])
	else
		__msg="not found, please specify correct value using '--with-$1-incl'"

		if test "x$cached_want_$1_incl" == "xyes"; then
			AC_MSG_ERROR([$__msg])
		fi

		AC_MSG_WARN([$__msg])

		unset __msg

		ifelse([$7],,[:],[$7])
	fi

])dnl


dnl DRV_INCL_CHECK_FOR_LL_DRV(
dnl [1-SHORT-NAME], [2-LONG-NAME],
dnl [3-INCL-VARIABLE], [4-INCL-IS_SET-VARIABLE])
dnl ----------------------------------------------------------
dnl
dnl Checks for driver includes for LL drivers
dnl specify --with-$drv-incl
dnl
AC_DEFUN([DRV_INCL_CHECK_FOR_LL_DRV],
[
   AM_CONDITIONAL($4, false)
   __$1_incl_path_set="no"
   dnl set driver include path
   AC_ARG_ENABLE($1incl,,
       [
         AC_MSG_WARN([Warning: using deprecated configure switch --enable-$1incl, use --with-$1-incl instead])
         __$1_incl_path=$enableval
         __$1_incl_path_set="yes"
       ]
   )
   AC_ARG_WITH($1-incl,
       AS_HELP_STRING(
           [--with-$1-incl=x],
           [set the $2 include path]
       ),
       [
         __$1_incl_path=$withval
         __$1_incl_path_set="yes"
       ]
   )
   AC_MSG_CHECKING(for $2 includes)
   dnl check if path was specified
   if test "x$__$1_incl_path_set" == "xyes"; then
      dnl check if path exists
      if test -d $__$1_incl_path; then
         dnl create an absolute path to be valid also from within the build dir
         if test "`echo $__$1_incl_path|cut -c1`" != "/" ; then
            __$1_incl_path=`echo $PWD/$__$1_incl_path`
         fi
         AC_MSG_RESULT([$__$1_incl_path])
         AC_SUBST([$3],[$__$1_incl_path])
         AM_CONDITIONAL($4, true)
      else
         AC_MSG_WARN([$__$1_incl_path is invalid])
      fi
   else
      AC_MSG_RESULT([assuming default include path. Change with --with-$1-incl=path])
   fi
   
   unset __$1_incl_path_set __$1_incl_path
])

dnl DRV_TAPI_INCL_CHECK_FOR_LL_DRV
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_tapi includes for LL drivers
dnl specify --with-tapi-incl
dnl
AC_DEFUN([DRV_TAPI_INCL_CHECK_FOR_LL_DRV],
[
   DRV_INCL_CHECK_FOR_LL_DRV(tapi, High-Level TAPI,
      HL_TAPI_INCL_PATH, HL_TAPI_INCL_PATH_SET)
])


dnl DRV_MPS_INCL_CHECK_FOR_LL_DRV
dnl ----------------------------------------------------------
dnl
dnl Checks for mps driver includes
dnl specify --with-mps-incl
dnl
AC_DEFUN([DRV_MPS_INCL_CHECK_FOR_LL_DRV],
[
   DRV_INCL_CHECK_FOR_LL_DRV(mps, MPS driver,
      MPS_INCL_PATH, MPS_INCL_PATH_SET)
])

dnl WITH_MPS_CHECK
dnl ----------------------------------------------------------
dnl
dnl Checks for mps driver includes
dnl specify --with-mps-incl
dnl
AC_DEFUN([WITH_MPS_CHECK],
[
   dnl handle common VMMC and MPS build - default is one common VMMC driver
   AC_DEFINE_UNQUOTED([CONFIG_MPS_EVENT_MBX], [1], [enable MPS event mailbox])
   AM_CONDITIONAL(VMMC_WITH_MPS, true)
   AC_ARG_WITH(mps,
       AS_HELP_STRING(
           [--with-mps=val],
           [build common VMMC and MPS driver]
       ),
       [
           if test $withval = 'yes' -o $withval = ''; then
              AC_MSG_RESULT([Configured for common VMMC and MPS driver])
              AC_DEFINE_UNQUOTED([VMMC_WITH_MPS], [1], [common VMMC and MPS build compile switch])
           else
              AC_MSG_RESULT([Configured for standalone VMMC without MPS])
              AM_CONDITIONAL(VMMC_WITH_MPS, false)
           fi
       ],
       [
          AC_MSG_RESULT([Configured for common VMMC and MPS driver (default)])
          AC_DEFINE_UNQUOTED([VMMC_WITH_MPS], [1], [common VMMC and MPS build compile switch])
       ]
   )
])

dnl
dnl Option used to set size of MPS buffer size
dnl -------------------------------------------------------
dnl MPS_HISTORY_CHECK
dnl
dnl available values are:
dnl		--enable-history-buf=val
dnl
AC_DEFUN([MPS_HISTORY_CHECK],
[
   MPS_HISTORY=128
	AC_MSG_CHECKING(for MPS history buffer size)
	AC_ARG_ENABLE(history-buf,
		AS_HELP_STRING(
         [--enable-history-buf=val],
         [MPS history buffer (default=128 words, maximum=512 words, 0=disable)]
		),
		[
         if test -z $enableval; then
            MPS_HISTORY=128
            AC_MSG_RESULT([no parameter, set to default (128 words) ATTENTION!])
         elif test $enableval -lt 0 -o $enableval -gt 512; then
            AC_MSG_ERROR([invalid value. Valid are 0(disable)..512 words ($enableval)])
         else
            MPS_HISTORY=$enableval
            AC_MSG_RESULT([$enableval words])
         fi
      ],
		[
         AC_MSG_RESULT([default value of $MPS_HISTORY words])
      ]
	)
   AC_DEFINE_UNQUOTED([CONFIG_MPS_HISTORY_SIZE],[$MPS_HISTORY],[MPS history buffer size in words])
])

dnl MAX_DEVICES_CHECK(
dnl  [1-NAME] [2-VARIABLE])
dnl ----------------------------------------------------------
dnl
dnlSet number of max devices
dnl specify --with-max-devices
dnl
dnl
AC_DEFUN([MAX_DEVICES_CHECK],
[
   MAX_DEVICES=1
   dnl set the maximum number of devices supported
   AC_MSG_CHECKING(for maximum number of devices supported)
   AC_ARG_WITH(max-devices,
       AS_HELP_STRING(
           [--with-max-devices=val],
           [maximum $1 devices to support.]
       ),
       [
          if test "$withval" = yes; then
             AC_MSG_ERROR([Please provide a value for the maximum devices]);
          fi
          AC_MSG_RESULT([$withval device(s)])
          MAX_DEVICES=$withval
       ],
       [
          AC_MSG_RESULT([1 device (default), set max devices with --with-max-devices=val])
       ]
   )
   dnl make sure this is defined even if option is not given!
   AC_DEFINE_UNQUOTED([$2],[$MAX_DEVICES],[Maximum $1 devices to support])
])

dnl PROC_CHECK(
dnl   [1-VARIABLE])
dnl ----------------------------------------------------------
dnl
dnl enable use of proc filesystem entries
dnl specify --enable-proc (enabled by default)
dnl
AC_DEFUN([PROC_CHECK],
[
   dnl enable use of proc filesystem entries
   AC_ARG_ENABLE(proc,
       AS_HELP_STRING(
           [--enable-proc],
           [enable use of proc filesystem entries]
       ),
       [
           if test $enableval = 'yes'; then
               AC_MSG_RESULT(enable use of proc filesystem entries (Linux only))
               AC_DEFINE([$1],[1],[enable use of proc filesystem entries (Linux only)])
           fi
       ],
       [
           AC_MSG_RESULT(enable use of proc filesystem entries (Linux only))
           AC_DEFINE([$1],[1],[enable use of proc filesystem entries (Linux only)])
       ]
   )
])

dnl EVALUATION_CHECK
dnl ----------------------------------------------------------
dnl
dnl enable evaluation features e.g. for testing with WinEASY
dnl specify --enable-eval
dnl
AC_DEFUN([EVALUATION_CHECK],
[
   dnl enable evaluation features
   AM_CONDITIONAL(EVALUATION, false)
   AC_ARG_ENABLE(eval,
       AS_HELP_STRING(
           [--enable-eval],
           [enable evaluation features]
       ),
       [
          AC_MSG_CHECKING(for evaluation features)
          if test $enableval = 'yes'; then
             AC_MSG_RESULT(enabled)
             AC_DEFINE([EVALUATION],[1],[enable evaluation features e.g. for testing with WinEASY])
             AM_CONDITIONAL(EVALUATION, true)
          else
             AC_MSG_RESULT(disabled)
          fi
       ]
   )
])

dnl USER_CONFIG_CHECK
dnl ----------------------------------------------------------
dnl
dnl enable user configuration
dnl specify --enable-user-config (disabled by default)
dnl
AC_DEFUN([USER_CONFIG_CHECK],
[
   dnl enable user configuration
   AC_ARG_ENABLE(user-config,
       AS_HELP_STRING(
           [--enable-user-config],
           [enable user configuration]
       ),
       [
          AC_MSG_RESULT(enable user configuration)
          AC_DEFINE([ENABLE_USER_CONFIG],[1],[enable user configuration.])
       ]
   )
])

dnl
dnl Option used to enable Analog Line Calibration support
dnl -------------------------------------------------------
dnl CALIBRATION_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-calibration
dnl		--disable-calibration (default)
dnl
AC_DEFUN([CALIBRATION_CHECK],
[
	AC_MSG_CHECKING(for Analog Line Calibration support)
	AC_ARG_ENABLE(calibration,
		AS_HELP_STRING(
         [--enable-calibration],
         [enable Analog Line Calibration support]
		),
		[__enable_calibration=$enableval],
		[__enable_calibration=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_calibration" = "yes" -o "$__enable_calibration" = "enable"; then
		AC_DEFINE_UNQUOTED([CALIBRATION_SUPPORT], [1], [enable Analog Line Calibration support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_calibration
])

dnl
dnl Option used to enable obsolete data channel premapping
dnl -------------------------------------------------------
dnl OBSOLETE_PREMAPPING_CHECK(
dnl [DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-obsolete-premapping
dnl		--disable-obsolete-premapping (default)
dnl
AC_DEFUN([OBSOLETE_PREMAPPING_CHECK],
[
	AC_ARG_ENABLE(obsolete-premapping,
		AS_HELP_STRING(
         [--enable-obsolete-premapping],
         [enable obsolete data channel premapping]
		),
		[__enable_obsolete_premaping=$enableval],
		[__enable_obsolete_premaping=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_obsolete_premaping" = "yes" -o "$__enable_obsolete_premaping" = "enable"; then
      AC_DEFINE([ENABLE_OBSOLETE_PREMAPPING],[1],[enable obsolete data channel premapping])
      AM_CONDITIONAL(ENABLE_OBSOLETE_PREMAPPING, true)
      AC_MSG_CHECKING(obsolete data channel premapping)
      AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
      AM_CONDITIONAL(ENABLE_OBSOLETE_PREMAPPING, false)

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_obsolete_premaping
])

dnl WITH_INCL_CHECK(
dnl [1-SHORT-NAME], [2-LONG-NAME],
dnl [3-INCL-VARIABLE], [4-FILE-NAME])
dnl ----------------------------------------------------------
dnl
dnl Another macro for checkin for xxx includes with filename
dnl specify --with-$xxx-incl
dnl
AC_DEFUN([WITH_INCL_CHECK],
[
   __$1_incl_path_set="no"
   dnl set driver include path
   AC_MSG_CHECKING(for $2 includes)
   AC_ARG_WITH($1-incl,
       AS_HELP_STRING(
           [--with-$1-incl=x],
           [set the $2 include path]
       ),
       [
         __$1_incl_path=$withval
         __$1_incl_path_set="yes"
       ],
       [
         AC_MSG_ERROR([path to $2 mandatory])
       ]
   )
   dnl check if path was specified
   if test "x$__$1_incl_path_set" == "xyes"; then
      dnl check if path exists
      if test -d $__$1_incl_path; then
         dnl create an absolute path to be valid also from within the build dir
         if test "`echo $__$1_incl_path|cut -c1`" != "/" ; then
            __$1_incl_path=`echo $PWD/$__$1_incl_path`
         fi
         dnl now check if file exists
         if test ! -r "${__$1_incl_path}/$5"; then
            dnl no such file
            AC_MSG_ERROR([$__$1_incl_path is invalid, no $5 file])
         else
            AC_MSG_RESULT([$__$1_incl_path])
            AC_SUBST([$3],[$__$1_incl_path])
         fi
      else
         AC_MSG_ERROR([$__$1_incl_path is invalid])
      fi
   fi

   unset __$1_incl_path_set __$1_incl_path
])

dnl WITH_BOARD_INCL_CHECK(
dnl [1-SHORT-NAME], [2-LONG-NAME],
dnl [3-INCL-VARIABLE], [4-INCL-IS_SET-VARIABLE], [5-FILE-NAME])
dnl ----------------------------------------------------------
dnl
dnl Another macro for checkin for xxx includes with filename
dnl specify --with-board-incl
dnl
AC_DEFUN([WITH_BOARD_INCL_CHECK],
[
  WITH_INCL_CHECK(
    [board],
    [Board Driver],
    [BOARD_DRV_INCL_PATH],
    [drv_board_io.h])
])


dnl DEVICE_ENABLE(
dnl   [1-DEV], [2-NAME], [3-ACTION IF PRESENT])
dnl ----------------------------------------------------------
dnl
dnl enable use support of given device
dnl specify --enable-[DEV]
dnl
AC_DEFUN([DEVICE_ENABLE],
[
   AC_MSG_CHECKING(for $1 support)
   AC_ARG_ENABLE($1,
       AS_HELP_STRING(
           [--enable-$1],
           [enable support for $2 device]
       ),
       [
           if test $enableval = 'yes'; then
               AC_MSG_RESULT([enabled])
               ifelse([$3],,[:],[$3])
           else
               AC_MSG_RESULT([disabled])
           fi
       ],
       [
           AC_MSG_RESULT([disabled])
       ]
   )
])
