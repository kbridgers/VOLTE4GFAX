dnl SVIP_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_svip
dnl specify --with-svip-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([SVIP_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[svip],
			[SVIP driver],
			[drv_svip drv_svip-* voice_svip_drv],
			[svip_io.h],
			[$1],
			[
				AC_SUBST([SVIP_INCL_PATH],[$cached_with_svip_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl


dnl Option used to enable SVIP Watchdog support
dnl -------------------------------------------------
dnl SVIP_WDT_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-wdt
dnl		--disable-wdt (default)
dnl
AC_DEFUN([SVIP_WDT_CHECK],
[
	AC_MSG_CHECKING(for SVIP Watchdog support)
	AC_ARG_ENABLE(wdt,
		AS_HELP_STRING(
			[--enable-wdt],
			[Enable SVIP firmware watchdog support]
		),
		[__enable_svip_wdt=$enableval],
		[__enable_svip_wdt=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_svip_wdt" = "yes" -o "$__enable_svip_wdt" = "enable"; then
		AC_DEFINE([SVIP_WDT],[1],[enable SVIP Watchdog support])
		AM_CONDITIONAL(WITH_WDT, true)
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AM_CONDITIONAL(WITH_WDT, false)
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_svip_wdt
])


dnl Option used to disable the MPS(mailbox) support inside the MPS driver, 
dnl with it one implicitly enables the SVIP WDT support inside the driver.
dnl This option is to be used for SVIP external LCC case.
dnl -------------------------------------------------
dnl SVIP_MPS_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-mps (default)
dnl		--disable-mps
dnl
AC_DEFUN([SVIP_MPS_CHECK],
[
	AC_MSG_CHECKING(for SVIP MPS support)
	AC_ARG_ENABLE(mps,
		AS_HELP_STRING(
			[--enable-mps],
			[Enable SVIP MPS support, disable when --enable-wdt is given for external LCC]
		),
		[__enable_svip_mps=$enableval],
		[__enable_svip_mps=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_svip_mps" = "yes" -o "$__enable_svip_mps" = "enable"; then
		AC_DEFINE([SVIP_MPS],[1],[enable SVIP MPS support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_svip_mps
])

