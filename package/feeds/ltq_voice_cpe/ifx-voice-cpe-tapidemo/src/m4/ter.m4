dnl TERIDIAN_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_ter1x66
dnl specify --with-teridian-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([TERIDIAN_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[teridian],
			[73M1x66 driver],
			[drv_ter1x66 drv_ter1x66-* voice_ter1x66_drv],
			[73m1966_io.h],
			[$1],
			[
				AC_SUBST([TERIDIAN_INCL_PATH],[$cached_with_teridian_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl

