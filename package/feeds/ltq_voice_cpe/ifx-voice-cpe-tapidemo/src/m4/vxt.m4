dnl VXT_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_vxt
dnl specify --with-vxt-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([VXT_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[vxt],
			[VINETIC-xT driver],
			[drv_vxt drv_vxt-* drv_vinetic_xt drv_vinetic_xt-* voice_xt_drv],
			[drv_vxt_io.h],
			[$1],
			[
				AC_SUBST([VXT_INCL_PATH],[$cached_with_vxt_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl
