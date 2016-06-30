dnl VMMC_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_vmmc
dnl specify --with-vmmc-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([VMMC_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[vmmc],
			[VMMC driver],
			[drv_vmmc drv_vmmc-* voice_vmmc_drv],
			[vmmc_io.h],
			[$1],
			[
				AC_SUBST([VMMC_INCL_PATH],[$cached_with_vmmc_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl

dnl VMMC_DEVICE_ENABLE([1- ACTION_IF_SET])
dnl ----------------------------------------------------------
dnl
dnl Set VMMC support for LT lib
dnl by specifing --enable-vmmc.
dnl
AC_DEFUN([VMMC_DEVICE_ENABLE],
[
    DEVICE_ENABLE([vmmc],[VMMC],[ifelse([$1],,[:],[$1])])

])dnl

