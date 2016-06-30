# LIBSVIP_CONFIG ([DEFAULT-INCL-PATH], [DEFAULT-LIB-PATH])
# ----------------------------------------------------------
#
# Checks for lib_ifxos 
# specify --with-ifxos-lib 
# and --with-ifxos-incl 
# If not supplied error results.
#


AC_DEFUN([LIBIFXOS_CHECK_CONFIG],
[
   dnl Determine user choice for ifxos library
   AC_ARG_ENABLE([ifxos-library],
      AS_HELP_STRING(
         [--enable-ifxos-library@<:@=DIR@:>@],
         [(deprecated) Include ifxos @<:@default='ifelse([$1],,[[./]],[$1])'@:>@. DIR is the path to the ifxos library.]),
   [
      AC_MSG_WARN([Warning: using deprecated configure switch --enable-ifxos-library])
      _want_libifxos_lib="yes"
      _libifxos_lib_with=$enableval
   ],[
      _want_libifxos_lib=ifelse([$1],,[no],[yes])
      _libifxos_lib_with=ifelse([$1],,[./],[$1])
   ])

   AC_ARG_WITH([ifxos-lib],
      AS_HELP_STRING(
         [--with-ifxos-lib@<:@=DIR@:>@],
         [Include ifxos @<:@default=yes@:>@. DIR is the path to the ifxos library.]),
   [
      _want_libifxos_lib="yes"
      _libifxos_lib_with=$withval
   ])
   
   dnl Determine user choice for ifxos includes
   AC_ARG_ENABLE([ifxos-include],
      AS_HELP_STRING(
         [--enable-ifxos-include@<:@=DIR@:>@],
         [(deprecated) Include ifxos @<:@default='ifelse([$2],,[[value from --enable-ifxos-include]],[$2])'@:>@. DIR is the path to the ifxos includes.]),
   [
		AC_MSG_WARN([Warning: using deprecated configure switch --enable-ifxos-include])
      _want_libifxos_incl="yes"
      _libifxos_incl_with=$enableval
   ],[
		_want_libifxos_incl=ifelse([$2],,[$_want_libifxos_lib],[yes])
		_libifxos_incl_with=ifelse([$2],,[$_libifxos_lib_with],[$2])
   ])

   AC_ARG_WITH([ifxos-incl],
      AS_HELP_STRING(
         [--with-ifxos-incl@<:@=DIR@:>@],
         [Include ifxos @<:@default='value from --with-ifxos-incl'@:>@. DIR is the path to the ifxos which contains the include dir.]),
   [
      _want_libifxos_incl="yes"
      _libifxos_incl_with=$withval
   ])

   dnl Set dafault value
dnl   if test "x$_libifxos_incl_with" == "xyes"; then
dnl      _libifxos_incl_with="/var/vob/comacsd/comacsd_lib/lib_ifxos/include"
dnl   fi
   
   dnl Set global choice
   if test "x$_want_libifxos_lib" != "xno" -a "x$_want_libifxos_incl" != "xno"; then
      want_libifxos="yes"
   elif test "x$_want_libifxos_lib" == "xno" -a "x$_want_libifxos_incl" != "xno"; then
      AC_MSG_ERROR([Path to the ifxos library is mandatory])
   elif test "x$_want_libifxos_lib" != "xno" -a "x$_want_libifxos_incl" == "xno"; then
      AC_MSG_ERROR([Path to the ifxos library is mandatory])
   fi
    
   found_libifxos="no"
   
	dnl Save current statement
	__saved_CFLAGS=$CFLAGS

	CFLAGS="$CFLAGS -I$_libifxos_incl_with"

   dnl Check path to the ifxos
   AC_MSG_CHECKING(for ifxos lib)
   if test -f $_libifxos_lib_with/libifxos.a; then
		AC_MSG_RESULT($_libifxos_lib_with)
	else
      AC_MSG_ERROR([Incorrect path to the ifxos library '$_libifxos_lib_with'])
   fi
   
   dnl Check path to the ifxos includes
   AC_MSG_CHECKING(for ifxos includes)
   if test -e $_libifxos_incl_with/ifxos_version.h; then
      AC_COMPILE_IFELSE(
            [AC_LANG_PROGRAM([[#include "ifxos_version.h"]],
                             [[return;]])],
            [AC_MSG_RESULT([$_libifxos_incl_with])],
            [AC_MSG_ERROR([Invalid ifxos_version.h file in '$_libifxos_incl_with'])]
         )
   else
      AC_MSG_ERROR([No ifxos_version.h file in '$_libifxos_incl_with'])
   fi

   IFXOS_INCL_PATH=$_libifxos_incl_with
   IFXOS_LIBRARY_PATH=$_libifxos_lib_with

   found_libifxos="yes"
   AC_DEFINE(HAVE_LIBIFXOS,1,[Define to 1 if ifxos should be enabled.])

   AC_SUBST(IFXOS_INCL_PATH)
   AC_SUBST(IFXOS_LIBRARY_PATH)

	dnl Restore saved statement
	CFLAGS=$__saved_CFLAGS

   unset _want_libifxos_incl _libifxos_incl_with
   unset _want_libifxos_lib _libifxos_lib_with 

])dnl

# LIBIFXOS_CHECK_CONFIG_INCL ([DEFAULT-INCL-PATH])
# ----------------------------------------------------------
#
# Checks for lib_ifxos includes
# specify --with-ifxos-incl 
# If not supplied error results.
#
AC_DEFUN([LIBIFXOS_CHECK_CONFIG_INCL],
[
   dnl Determine user choice for ifxos includes
   AC_ARG_ENABLE([ifxos-include],,
   [
		AC_MSG_WARN([Warning: using deprecated configure switch --enable-ifxos-include])
      _set_libifxos_incl="yes"
      _libifxos_incl_path=$enableval
   ],[
		_set_libifxos_incl="no"
   ])

   AC_ARG_WITH([ifxos-incl],
      AS_HELP_STRING(
         [--with-ifxos-incl@<:@=DIR@:>@],
         [Include ifxos @<:@default='value from --with-ifxos-incl'@:>@. DIR is the path to the ifxos which contains the include dir.]),
   [
      _set_libifxos_incl="yes"
      _libifxos_incl_path=$withval
   ])
   
   dnl Set global choice
   if test "x$_set_libifxos_incl" == "xno"; then
      AC_MSG_ERROR([Path to the ifxos library includes is mandatory])
   fi
   
   if test ! -r $_libifxos_incl_path/ifx_types.h; then
      AC_MSG_ERROR([The lib_ifxos include directory is not valid! '$_libifxos_incl_path'])
   fi
   
	dnl Save current statement
	__saved_CFLAGS=$CFLAGS

	CFLAGS="$CFLAGS -I$_libifxos_incl_path"
   
   dnl Check path to the ifxos includes
   AC_MSG_CHECKING(for ifxos includes)
   if test -e $_libifxos_incl_path/ifxos_version.h; then
      AC_COMPILE_IFELSE(
            [AC_LANG_PROGRAM([[#include "ifxos_version.h"]],
                             [[return;]])],
            [AC_MSG_RESULT([$_libifxos_incl_path])],
            [AC_MSG_ERROR([Invalid ifxos_version.h file in '$_libifxos_incl_path'])]
         )
   else
      AC_MSG_ERROR([No ifxos_version.h file in '$_libifxos_incl_path'])
   fi

   AC_SUBST([IFXOS_INCL_PATH], [$_libifxos_incl_path])

	dnl Restore saved statement
	CFLAGS=$__saved_CFLAGS

   unset _set_libifxos_incl _libifxos_incl_path

])dnl


