dnl
dnl                              Copyright (c) 2012
dnl                            Lantiq Deutschland GmbH
dnl                             http://www.lantiq.com
dnl
dnl  For licensing information, see the file 'LICENSE' in the root folder of
dnl  this software module.
dnl
dnl

dnl  Enable EASY 3111 extension board for vmmc boards.
dnl -------------------------------------------------
dnl --enable-dxt-ext - argument must be used for configure.in
AC_DEFUN([EASY3111_EXTENSION_CHECK],
[
	dnl Enable EASY 3111 as extension board for AR9/GR9 and VR9
	AC_MSG_CHECKING(for EASY 3111)
	AC_ARG_ENABLE(dxt-ext,
		AS_HELP_STRING(--enable-dxt-ext, Enable EASY 3111 as extension board for AR9/GR9 and VR9.),
		[test "x$enableval" = "xyes" && with_easy3111=true]
	)
	if [[ "x$with_easy3111" = "xtrue" ]]; then
		AC_MSG_RESULT([enabled])
		DXT_INCL_CHECK(,
		[
			AC_DEFINE([HAVE_DRV_DUSLICXT_HEADERS],[1],[using drv_duslic_xt])
			AC_DEFINE([DXT],[1],[Duslic-xT is used])
			AC_DEFINE([EASY3111],[1],[Enable EASY3111 support])
			AM_CONDITIONAL(DEV_DXT, true)
		],
		[AC_MSG_WARN([EASY 3111 disabled because of Duslic-XT driver was not found])])
	else
		AC_MSG_RESULT([disabled])
	fi
])

dnl  Enable IPv6 support.
dnl -------------------------------------------------
dnl --disable-ipv6 - argument must be used for configure.in
AC_DEFUN([TD_IPV6_CHECK],
[
   dnl Enable EASY 3111 as extension board for AR9/GR9 and VR9
	AC_MSG_CHECKING(for IPv6)
   AC_ARG_ENABLE(td-ipv6,
      AS_HELP_STRING(
         [--disable-td-ipv6],
         [disable ipv6 support]
      ),
      [
         if test "x$enableval" = "xno"; then
            AC_MSG_RESULT([disabled])
         else
            AC_MSG_RESULT([enabled])
            AC_DEFINE([TD_IPV6_SUPPORT],[1],[enable IPv6 support])
         fi
      ],[
            AC_MSG_RESULT([enabled(default)])
            AC_DEFINE([TD_IPV6_SUPPORT],[1],[enable IPv6 support])
      ]
   )
])

dnl
dnl Option used to enable Quality of Service and UDP redirection (QoS)
dnl -------------------------------------------------
dnl TD_QOS_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-qos
dnl		--disable-qos (default)
dnl
AC_DEFUN([TD_QOS_CHECK],
[
	AC_MSG_CHECKING(for Quality of Service and UDP redirection)
	AC_ARG_ENABLE(qos,
		AS_HELP_STRING(
         [--enable-qos],
         [enable QoS - quality of service and UDP redirection]
		),
		[__enable_td_qos=$enableval],
		[__enable_td_qos=ifelse([$1],,[no],[$1])]
	)
   AM_CONDITIONAL(QOS_SUPPORT, false)
	if test "$__enable_td_qos" = "yes" -o "$__enable_td_qos" = "enable"; then
      #store current CPPFLAGS value
      __old_CPPFLAGS="$CPPFLAGS"

      #add tapi includes
      CPPFLAGS="$CPPFLAGS -I$TAPI_INCL_PATH"

      #check QOS header file availability
      AC_CHECK_HEADERS([drv_tapi_qos_io.h],
      [
         AC_DEFINE([QOS_SUPPORT],[1],[enable QOS support])
         AM_CONDITIONAL(QOS_SUPPORT, true)
         AC_MSG_RESULT([enabled])

         ifelse([$2],,[:],[$2])      
      ],
      [
         AC_MSG_RESULT([disabled])
         AC_MSG_WARN([No drv_tapi_qos_io.h file: unable to compile tapidemo with QoS support.])
      ],[#include "ifx_types.h"]
      )
      #restore CPPFLAGS value
      CPPFLAGS="$__old_CPPFLAGS"
      
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_td_qos __old_CPPFLAGS
])


dnl
dnl Add include/library path with backward compatibility
dnl -------------------------------------------------
dnl TD_DECT_CHECK(
dnl [1-WIITH-CONFIGURE], [2-ENABLE-CONFIGURE], [3-SHORT-NAME]
dnl [4-LIB-INCLUDE], [5-DIR-DEFAULT], [6-DIR-TO-CHECK],
dnl [7-FILES-TO-CHECK], [8-PATH-NAME-TO-SET])
dnl
AC_DEFUN([TD_INCL_CHECK_WITH_BW_COMP],
[
   _$3_path=$5
   _$3_path_was_set="no"
   dnl check outdated 
   AC_ARG_ENABLE($2,,
		[
         AC_MSG_WARN([Warning: using deprecated configure switch --enable-$2])
         _$3_path="$enableval"
         _$3_path_was_set="yes"
		]
   )
   dnl but this is the option that should be used
   AC_ARG_WITH($1,
		AS_HELP_STRING(
			[--with-$1=I/path/to/$3/$4],
			[Set the $3 $4 path]
		),
		[
         _$3_path="$withval"
         _$3_path_was_set="yes"
		]
	)
   
   dnl create an absolute path to be valid also from within the build dir
   if test "`echo ${_$3_path}|cut -c1`" != "/" ; then
      _$3_path=`echo $PWD/${_$3_path}`
   fi
   
   dnl validate path
	AC_MSG_CHECKING(for $3 $4)
   _$3_valid_path="yes"
   dnl is dir existing
   if test -d "${_$3_path}/$6"; then
      dnl for all files check if they exist
      for _$3_file in $7; do
         dnl is file existing
         if test ! -r "${_$3_path}/$6/$_$3_file"; then
            dnl no such file
            _$3_valid_path="no"
         fi
      done
   else
      dnl no such directory
      _$3_valid_path="no"
   fi
   
   dnl check if path was found
   if test "x$_$3_valid_path" == "xyes"; then
      dnl it is all OK - set the path
      AC_SUBST([$8],[$_$3_path])
   else
      dnl print error
      if test "x$_$3_path_was_set" == "xyes"; then
         AC_MSG_ERROR([The $3 $4 directory ${_$3_path}$6 is not valid.])
      else
         AC_MSG_ERROR([Please specify the $3 $4 directory path. (--with-$1=x)])
      fi
   fi
	AC_MSG_RESULT([${$8}])
   
   unset _$3_valid_path _$3_path _$3_path_was_set _$3_file
])


AC_DEFUN([TD_DECT_INCL_CHECK],
[
   TD_INCL_CHECK_WITH_BW_COMP(dect-incl, dect-incl, dect,
      include, "$STAGING_DIR/usr/include", ifx-dect,
      IFX_DECT_Stack.h, DECT_INCL_PATH)
])

AC_DEFUN([TD_DECT_LIB_CHECK],
[
   TD_INCL_CHECK_WITH_BW_COMP(dect-lib, dect-lib, dect,
      library, "$STAGING_DIR/usr/lib", ,
      ["libdtk.a" "libdectstack.a"], DECT_LIBRARY_PATH)
])

AC_DEFUN([TD_VOIP_COMMON_INCL_CHECK],
[
   TD_INCL_CHECK_WITH_BW_COMP(voip-common-incl, voip-common-incl, voip_common,
      include, "$STAGING_DIR/usr/include", ifx-voip-common,
      IFX_TLIB_Timlib.h, VOIP_COMMON_INCL_PATH)
])

AC_DEFUN([TD_VOIP_COMMON_LIB_CHECK],
[
   TD_INCL_CHECK_WITH_BW_COMP(voip-common-lib, voip-common-lib, voip_common,
      library, "$STAGING_DIR/usr/lib", ,
      libTimer.a, VOIP_COMMON_LIBRARY_PATH)
])

