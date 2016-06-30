dnl EASY3332_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_easy3332
dnl specify --with-easy3332-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([EASY3332_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[easy3332],
			[EASY3332 driver],
			[drv_easy3332 drv_easy3332-*],
			[easy3332_io.h],
			[$1],
			[
				AC_SUBST([EASY3332_INCL_PATH],[$cached_with_easy3332_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl

