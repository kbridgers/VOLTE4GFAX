dnl
dnl KERNEL_CHECK_CONFIG
dnl ----------------------------------------------------------
dnl
dnl Checks for kernel configuretion
dnl specify --with-kernel-incl, --with-kernel-build and --enable-linux-26
dnl
AC_DEFUN([KERNEL_CHECK_CONFIG],
[
	dnl check for kernel includes
	KERNEL_INCL_CHECK

   dnl check for linux kernel 2.6.x support
	KERNEL_2_6_CHECK(,[
		KERNEL_BUILD_CHECK
	])
])

dnl
dnl Option used to enable Linux kernel 2.6 support
dnl -------------------------------------------------
dnl KERNEL_2_6_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-linux-26
dnl		--disable-linux-26 (default)
dnl
dnl note: Make sure AC_PROG_CC is defined before to set $CC!
dnl
AC_DEFUN([KERNEL_2_6_CHECK],
[
	AC_MSG_CHECKING(for Linux kernel 2.6)

	if test "${cached_enable_linux26+set}" != set; then
		AC_ARG_ENABLE(linux-26,
			AS_HELP_STRING(
				[--enable-linux-26],
				[enable support for Linux kernel 2.6.x]
			),
			[__enable_linux26=$enableval],
			[__enable_linux26=ifelse([$1],,[no],[$1])]
		)

		if test "$__enable_linux26" = "yes" -o "$__enable_linux26" = "enable"; then
			AM_CONDITIONAL(KERNEL_2_6, true)
			AC_DEFINE([LINUX_2_6],[1],[enable linux 2.6 code])
			with_linux26=yes
			cached_enable_linux26=enabled
			AC_MSG_RESULT([enabled])

                        if test -z "$ARCH" ; then
                           [ARCH=`$CC -dumpmachine`]
                        fi
                        dnl
                        dnl FIX: assume ARCH=ppc in case if --host=powerpc-linux*,
                        dnl but arch/powerpc/Makefile is not present.
                        dnl Works for linux 2.6.x kernel releases < 2.6.15
                        dnl
                        if test `echo $ARCH | sed -e s'/-.*//'` = powerpc ; then
                           KERNEL_INCL_CHECK
                           if test ! -f $KERNEL_INCL_PATH/../arch/powerpc/Makefile ; then
                              [ARCH=ppc]
                           fi
                        fi
			AC_MSG_CHECKING(for kernel architecture)
			if test -n "$ARCH" ; then
				[ ARCH=`echo $ARCH | sed -e s'/-.*//' \
					-e 's/i[3-9]86/i386/' \
					-e 's/mipsel/mips/' \
					-e 's/sh[234]/sh/' \
				`]
				AC_SUBST([KERNEL_ARCH],[$ARCH])
				AC_MSG_RESULT([$ARCH])
			else
				AC_MSG_ERROR([Kernel architecture not set!])
			fi
		else
			AM_CONDITIONAL(KERNEL_2_6, false)
			with_linux26=no
			cached_enable_linux26=disabled
			AC_MSG_RESULT([disabled])
		fi

		unset __enable_linux26
	else
		AC_MSG_RESULT([$cached_enable_linux26 (cached)])
	fi

	if test "$cached_enable_linux26" = enabled; then
		ifelse([$2],,[:],[$2])
	else
		ifelse([$3],,[:],[$3])
	fi
])

dnl
dnl Option used to check for valid path to the kernel includes
dnl -------------------------------------------------
dnl KERNEL_INCL_CHECK
dnl
dnl available values are:
dnl		--with-kernel-incl=<path>
dnl
AC_DEFUN([KERNEL_INCL_CHECK],
[
	AC_MSG_CHECKING(for kernel includes)

	if test "${cached_with_kernel_incl+set}" != set; then
		dnl Check for user given kernel includes path
		AC_ARG_WITH([kernel-incl],
			AS_HELP_STRING(
				[--with-kernel-incl@<:@=DIR@:>@],
				[Target kernel sources path]
			),
			[__kernel_incl=$withval],
			[
				dnl Check for existance of depricated option --enable-kernelincl
				AC_ARG_ENABLE([kernelincl],,
					[
						AC_MSG_WARN([deprecated configure option --enable-kernelincl used, use --with-kernel-incl instead])
						__kernel_incl=$enableval
					],
                  [AC_MSG_ERROR([Set path to kernel includes using --with-kernel-incl@<:@=DIR@:>@])]
					)

			])
		AC_MSG_RESULT($__kernel_incl)

		dnl Verify if user given kernel includes path is valid
		if test -f $__kernel_incl/linux/kernel.h; then
			AC_SUBST([KERNEL_INCL_PATH],[$__kernel_incl])
			cached_with_kernel_incl=$__kernel_incl
		else
			AC_MSG_ERROR([Incorrect path to kernel includes '$__kernel_incl'])
		fi
	else
		AC_MSG_RESULT([$cached_with_kernel_incl (cached)])
	fi
])

dnl
dnl Option used to check for valid path to the kernel build path
dnl -------------------------------------------------
dnl KERNEL_BUILD_CHECK
dnl
dnl available values are:
dnl		--with-kernel-build=<path>
dnl
AC_DEFUN([KERNEL_BUILD_CHECK],
[
	AC_MSG_CHECKING(for kernel build path)

	if test "${cached_with_kernel_build+set}" != set; then

		dnl Check for user given kernel build path (used for Linux 2.6 only)
		AC_ARG_WITH([kernel-build],
			AS_HELP_STRING(
				[--with-kernel-build@<:@=DIR@:>@],
				[Target kernel build path, applies to Linux 2.6 version, only]
			),
			[__with_kernel_build=$withval],
			[
            __with_kernel_build=ifelse([$1],,[$KERNEL_INCL_PATH/..],[$1])
            dnl Check for existance of depricated option --enable-kernelbuild
            AC_ARG_ENABLE([kernelbuild],,
            [
               AC_MSG_WARN([deprecated configure option --enable-kernelbuild, use --with-kernel-build instead])
               __with_kernel_build=$enableval
            ])
            dnl Check for existance of depricated option --enable-kernel-build
            AC_ARG_ENABLE([kernel-build],,
            [
               AC_MSG_WARN([deprecated configure option --enable-kernel-build, use --with-kernel-build instead])
               __with_kernel_build=$enableval
            ])
         ]
		)

		AC_MSG_RESULT($__with_kernel_build)

		if test $with_linux26 != "yes"; then
			AC_MSG_WARN([Option --with-kernel-build used only together with option --enable-linux-26])
		fi

		if test -e $__with_kernel_build; then
                 dnl check if autoconf.h file is present
                 if test -r $__with_kernel_build/include/linux/autoconf.h; then
                  AC_SUBST([KERNEL_BUILD_PATH],[$__with_kernel_build])
                  cached_with_kernel_build=$__with_kernel_build
                 else
                  if test -r $__with_kernel_build/include/generated/autoconf.h; then
                   AC_SUBST([KERNEL_BUILD_PATH],[$__with_kernel_build])
                   cached_with_kernel_build=$__with_kernel_build
                  else
                   AC_MSG_ERROR([The kernel build directory is not valid or not configured!])
                  fi
                 fi
		else
			AC_MSG_ERROR([Incorrect path to kernel build directory])
		fi

		unset __with_kernel_build
	else
		AC_MSG_RESULT([$cached_with_kernel_build (cached)])
	fi
])

AC_DEFUN([KERNEL_MODULE_CHECK],
[
	dnl enable LINUX MODULE support
	AC_MSG_CHECKING(for Linux module support)
	AC_ARG_ENABLE(module,
		 AS_HELP_STRING(
			  [--enable-module],
			  [enable LINUX MODULE support]
		 ),
		 [
			  if test $enableval = 'yes'; then
					AC_MSG_RESULT(enabled)
					AC_DEFINE([USE_MODULE],[1],[enable LINUX MODULE support])
					AM_CONDITIONAL(USE_MODULE, true)
			  else
					AC_MSG_RESULT(disabled)
					AM_CONDITIONAL(USE_MODULE, false)
			  fi
		 ],
		 [
			  dnl Enable automatically:
			  AC_MSG_RESULT([enabled (default), disable with --disable-module])
			  AC_DEFINE([USE_MODULE],[1],[enable LINUX MODULE support])
			  AM_CONDITIONAL(USE_MODULE, true)
		 ]
	)
])

dnl
dnl Option used to configure build with or without kernel module
dnl -------------------------------------------------
dnl WITH_KERNEL_MODULE_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--with-kernel-module (default)
dnl		--without-kernel-module
dnl
AC_DEFUN([WITH_KERNEL_MODULE_CHECK],
[
	AC_MSG_CHECKING(for build with or without Kernel Module)
	AC_ARG_WITH(kernel-module,
		AS_HELP_STRING(
         [--with-kernel-module],
			[Build with or without Kernel Module]
		),
		[__with_kernel_module=$withval],
		[__with_kernel_module=ifelse([$1],,[yes],[$1])]
	)

	if test "$__with_kernel_module" = "yes" -o "$__with_kernel_module" = "1"; then
		AM_CONDITIONAL(WITH_KERNEL_MODULE, true)
		AC_SUBST([WITH_KERNEL_MODULE],[yes])
		AC_MSG_RESULT([with])

		ifelse([$2],,[:],[$2])

		KERNEL_CHECK_CONFIG
	else
		AM_CONDITIONAL(WITH_KERNEL_MODULE, false)
		AC_SUBST([WITH_KERNEL_MODULE],[no])
		AC_MSG_RESULT([without])

		ifelse([$3],,[:],[$3])
	fi

	unset __with_kernel_module
])

