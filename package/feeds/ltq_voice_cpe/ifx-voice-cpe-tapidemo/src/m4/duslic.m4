dnl DUSLIC_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_duslic
dnl specify --with-duslic-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([DUSLIC_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[duslic],
			[Duslic driver],
			[drv_duslic drv_duslic-* voice_duslic_drv],
			[drv_duslic_io.h],
			[$1],
			[
				AC_SUBST([DUSLIC_INCL_PATH],[$cached_with_duslic_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl

