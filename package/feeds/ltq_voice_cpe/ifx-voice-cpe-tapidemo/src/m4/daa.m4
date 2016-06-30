dnl DAA_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_daa
dnl specify --with-daa-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([DAA_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[daa],
			[DAA driver],
			[drv_daa drv_daa-* voice_daa_drv],
			[drv_daa.h],
			[$1],
			[
				AC_SUBST([DAA_INCL_PATH],[$cached_with_daa_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)

])dnl

