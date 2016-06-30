dnl EASY3201_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_easy3201
dnl specify --with-easy3201-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([EASY3201_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[easy3201],
			[EASY3201 driver],
			[drv_easy3201 drv_easy3201-*],
			[drv_easy3201_io.h],
			[$1],
			[
				AC_SUBST([EASY3201_INCL_PATH],[$cached_with_easy3201_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl

