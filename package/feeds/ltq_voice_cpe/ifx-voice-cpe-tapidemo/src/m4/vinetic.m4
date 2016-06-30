dnl VINETIC_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_vinetic
dnl specify --with-vinetic-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([VINETIC_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[vinetic],
			[VINETIC-CPE driver],
			[drv_vinetic drv_vinetic -* drv_peb3324 voice_vcpe_drv],
			[vinetic_io.h],
			[$1],
			[
				AC_SUBST([VINETIC_INCL_PATH],[$cached_with_vinetic_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl

dnl VINCPE_DEVICE_ENABLE([1- ACTION_IF_SET])
dnl ----------------------------------------------------------
dnl
dnl Set VINCPE support for LT lib
dnl by specifing --enable-vincpe.
dnl
AC_DEFUN([VINCPE_DEVICE_ENABLE],
[
    DEVICE_ENABLE([vincpe],[VINCPE],[ifelse([$1],,[:],[$1])])

])dnl
