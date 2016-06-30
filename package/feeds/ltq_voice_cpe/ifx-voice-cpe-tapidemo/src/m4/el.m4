
# EVENT_LOGGER_CHECK_CONFIG
# ----------------------------------------------------------
#
# Checks for evt_logger
# specify --enable-el_debug and --with-el-incl
# If not supplied it checks for default and returns error when
# the header file was not found.

AC_DEFUN([EVENT_LOGGER_CHECK_CONFIG],
[
   dnl determine absolute path to the srcdir
   if [[ -z "$abs_srcdir" ]]; then
      case $srcdir in
        .) abs_srcdir=$(pwd) ;;
        [[\\/]]* | ?:[[\\/]]* ) abs_srcdir=$srcdir ;;
        *) abs_srcdir=$(pwd)/$srcdir ;;
      esac
   fi
   
   AM_CONDITIONAL(EVENT_LOGGER_DEBUG, false)
   dnl enable Event Logger debugging
   AC_MSG_CHECKING(for Event Logger debugging)
   event_logger_debugging="no"
   AC_ARG_ENABLE(el_debug,
       AS_HELP_STRING(
         [--enable-el_debug],
         [enable event logger debugging (requires additional eventlogger package)]
      ),
      [
         if test $enableval = 'yes'; then
            AC_MSG_RESULT([enabled])
            AC_DEFINE([EVENT_LOGGER_DEBUG],[1],[enable event logger debugging])
            AM_CONDITIONAL(EVENT_LOGGER_DEBUG, true)
            event_logger_debugging="yes"
         else
            AC_MSG_RESULT([disabled])
         fi
      ],
      [
         AC_MSG_RESULT([disabled (default), enable it with --enable-el_debug])
      ]
   )

   dnl set Event Logger includes path
   AC_SUBST([EVENT_LOGGER_INCL_PATH],[$abs_srcdir/../event_logger/include])
   AC_ARG_WITH(el-incl,
      AS_HELP_STRING(
         [--with-el-incl@<:@=DIR@:>@],
         [Event Logger includes path, applied only with option --enable-el_debug]
      ),
      [
         AC_MSG_CHECKING(for valid path to Event Logger includes)
         if test -e $withval/el_log_macros.h; then
            AC_MSG_RESULT(yes)
            AC_SUBST([EVENT_LOGGER_INCL_PATH],[$withval])
         else
	         if test -e $withval/include/el_log_macros.h; then
	            AC_MSG_RESULT(yes)
	            AC_SUBST([EVENT_LOGGER_INCL_PATH],[$withval/include])
	         else
	            AC_MSG_RESULT(no)
	            AC_MSG_ERROR([Event Logger includes path '$withval' is invalid!])
	         fi
         fi
      ],
      [
         if test $event_logger_debugging == "yes"; then
            AC_MSG_CHECKING(for validity of default path to Event Logger includes)
            if test -e $EVENT_LOGGER_INCL_PATH/el_log_macros.h; then
               AC_MSG_RESULT(yes)
            else
               AC_MSG_RESULT(no)
               AC_MSG_ERROR([Event Logger default includes path '$EVENT_LOGGER_INCL_PATH' is invalid!])
            fi
         fi
      ]
   )
])
