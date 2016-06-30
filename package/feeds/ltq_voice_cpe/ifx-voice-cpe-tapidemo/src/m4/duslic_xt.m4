dnl Option used to enable DuSLIC-xT power saving mode
dnl -------------------------------------------------
dnl DXT_POWER_SAVE_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-power-save (default)
dnl		--disable-power-save
dnl
AC_DEFUN([DXT_POWER_SAVE_CHECK],
[
	AC_MSG_CHECKING(for DuSLIC-xT power saving support)
	AC_ARG_ENABLE(power-save,
		AS_HELP_STRING(
			[--enable-power-save],
			[enable power saving support, default enabled, disabling is experimental]
		),
		[__enable_dxt_power_save=$enableval],
		[__enable_dxt_power_save=ifelse([$1],,[yes],[$1])]
	)
        
	if test "$__enable_dxt_power_save" = "yes" -o "$__enable_dxt_power_save" = "enable"; then
		AC_DEFINE([DXT_POWER_SAVE],[1],[enable DuSLIC-xT power saving support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_dxt_power_save
])

dnl DXT_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_duslic_xt
dnl specify --with-dxt-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([DXT_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[dxt],
			[Duslic-XT driver],
			[drv_dxt drv_dxt-* drv_duslic_xt drv_duslic_xt-* voice_duslic_xt_drv],
			[drv_dxt_io.h],
			[$1],
			[
				AC_SUBST([DUSLICXT_INCL_PATH],[$cached_with_dxt_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl

dnl DXT_SET_CHIP_VERSION()
dnl ----------------------------------------------------------
dnl
dnl Set supported chip versions - more than one version at once can be supported
dnl --enable-chip-v1_3 - support version 1.3
dnl --enable-chip-v1_4 - support version 1.4
dnl
AC_DEFUN([DXT_SET_CHIP_VERSION], [
   dnl set default values
   __enable_v13_support="no"
   __enable_v14_support="yes"
	AC_ARG_ENABLE(chip-v1_3,
		AS_HELP_STRING(
			[--enable-chip-v1_3],
			[enable DuSLIC-xT chip v1.3 support, default disabled]
		),
		[__enable_v13_support="$enableval"],
		[
      if test "x$enableval" = "xno"; then
         __enable_v13_support="$enableval"
      fi
   ])
	AC_ARG_ENABLE(chip-v1_4,
		AS_HELP_STRING(
			[--enable-chip-v1_4],
			[enable DuSLIC-xT chip v1.4 support, default enabled]
		),
		[__enable_v14_support="$enableval"],
		[
      if test "x$enableval" = "xno"; then
         __enable_v14_support="$enableval"
      fi
   ])

   AC_MSG_CHECKING(for DuSLIC-xT chip v1.3 support)
   if test "x$__enable_v13_support" = "xyes"; then
      AC_DEFINE([DXT_CHIP_V_1_3_SUPPORT],[1],
         [enable DuSLIC-xT chip v1.3 support])
      AC_MSG_RESULT([enabled])
   else
      AC_MSG_RESULT([disabled])
   fi

   AC_MSG_CHECKING(for DuSLIC-xT chip v1.4 support)
   if test "x$__enable_v14_support" = "xyes"; then
      AC_DEFINE([DXT_CHIP_V_1_4_SUPPORT],[1],
         [enable DuSLIC-xT chip v1.4 support])
      AC_MSG_RESULT([enabled])
   else
      AC_MSG_RESULT([disabled])
   fi

   unset __enable_v13_support __enable_v14_support
])dnl


dnl DXT_DEVICE_ENABLE([1- ACTION_IF_SET])
dnl ----------------------------------------------------------
dnl
dnl Set DXT support for LT lib
dnl by specifing --enable-dxt.
dnl
AC_DEFUN([DXT_DEVICE_ENABLE],
[
    DEVICE_ENABLE([dxt],[DXT],[ifelse([$1],,[:],[$1])])

])dnl

