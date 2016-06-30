dnl LIBSVIP_INCL_CHECK ([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for lib_svip
dnl specify --with-libsvip-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([LIBSVIP_INCL_CHECK],
[
   dnl determine absolute path to the srcdir
   if [[ -z "$abs_srcdir" ]]; then
      case $srcdir in
        .) abs_srcdir=$(pwd) ;;
        [[\\/]]* | ?:[[\\/]]* ) abs_srcdir=$srcdir ;;
        *) abs_srcdir=$(pwd)/$srcdir ;;
      esac
   fi

   __want_libsvip_incl="no"
   __found_libsvip_incl="no"

   AC_MSG_CHECKING(for SVIP library includes)
   AC_ARG_WITH([libsvip-incl],
      AS_HELP_STRING(
         [--with-libsvip-incl@<:@=DIR@:>@],
         [Include SVIP @<:@default=<srcdir>/../svip/common@:>@. DIR is path to SVIP library includes.]),
		 [
         __with_libsvip_incl=$withval
         __want_libsvip_incl="yes"
       ],
		[__with_libsvip_incl=ifelse([$1],,[$abs_srcdir/../svip/common],[$1])]
	)

	if test "x$__with_libsvip_incl" != "xno"; then
		if test -f "$__with_libsvip_incl/lib_svip.h"; then
			__found_libsvip_incl="yes"
		elif test -f "$__with_libsvip_incl/include/lib_svip.h"; then
			__found_libsvip_incl="yes"
			__with_libsvip_incl=${__with_libsvip_incl}/include
		fi
	fi

	AC_MSG_RESULT([$__with_libsvip_incl ($__found_libsvip_incl)])
	AC_SUBST([LIBSVIP_INCL_PATH],[$__with_libsvip_incl])

	if test "x$__found_libsvip_incl" == "xyes"; then
		ifelse([$2],,:,[$2])
	else
		__msg="not found, please specify correct value using '--with-libsvip-incl'"

		if test "x$__want_libsvip_incl" == "xyes"; then
			AC_MSG_ERROR([$__msg])
		fi

		AC_MSG_WARN([$__msg])

		unset __msg

		ifelse([$3],,:,[$3])
	fi

	unset __found_libsvip_incl __with_libsvip_incl __want_libsvip_incl
])dnl

dnl LIBSVIP_LIB_CHECK ([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for lib_svip
dnl specify --with-libsvip-lib
dnl If not supplied error results.
dnl
AC_DEFUN([LIBSVIP_LIB_CHECK],
[
   dnl determine absolute path to the srcdir
   if [[ -z "$abs_srcdir" ]]; then
      case $srcdir in
        .) abs_srcdir=$(pwd) ;;
        [[\\/]]* | ?:[[\\/]]* ) abs_srcdir=$srcdir ;;
        *) abs_srcdir=$(pwd)/$srcdir ;;
      esac
   fi

	__want_libsvip_lib="no"
	__found_libsvip_path="no"
	__found_libsvip_lib="no"

   AC_MSG_CHECKING(for SVIP library)
   dnl Determine user choice for SVIP library
   AC_ARG_WITH([libsvip-lib],
      AS_HELP_STRING(
         [--with-libsvip-lib@<:@=DIR@:>@],
         [Include libsvip @<:@default=<srcdir>/../svip/common@:>@. DIR is the path to the SVIP library.]),
		[
			__want_libsvip_lib="yes"
			__with_libsvip_lib=$withval
		],
		[__with_libsvip_lib=ifelse([$1],,[$abs_srcdir/../svip/common],[$1])]
   )

	if test "x$__with_libsvip_lib" != "xno"; then
		if test -f "$__with_libsvip_lib/libsvip.a"; then
			__found_libsvip_path="yes"
			__found_libsvip_lib="yes"
		elif test -f "$__with_libsvip_lib/src/libsvip.a"; then
			__found_libsvip_path="yes"
			__found_libsvip_lib="yes"
			__with_libsvip_lib=${__with_libsvip_lib}/src
		elif test -d "$__with_libsvip_lib"; then
			__found_libsvip_path="yes"
			__found_libsvip_lib="no"
		fi
	fi

	AC_MSG_RESULT([$__with_libsvip_lib ($__found_libsvip_lib)])
	AC_SUBST([LIBSVIP_LIBRARY_PATH],[$__with_libsvip_lib])

	if test "x$__found_libsvip_path" == "xyes"; then
		if test "x$__want_libsvip_lib" == "xyes" -a "x$__found_libsvip_lib" == "xno"; then
			AC_MSG_WARN([SVIP library path do not contain built library])
		fi

		ifelse([$2],,:,[$2])
	else
		__msg="not found, please specify correct value using '--with-libsvip-lib'"

		if test "x$__want_libsvip_lib" == "xyes"; then
			AC_MSG_ERROR([$__msg])
		fi

		AC_MSG_WARN([$__msg])

		unset __msg

		ifelse([$3],,:,[$3])
	fi

   unset __found_libsvip_path __found_libsvip_lib __want_libsvip_lib __with_libsvip_lib

])dnl
