# Option used to select an active VxWorks distribution type
# ---------------------------------------------------------
# available distribution type are:
#		--with-dist=vxworks6.4
#		--with-dist=vxworks6.6 (default)
#
AC_DEFUN([DISTRIBUTION_TYPE_CHECK],
[
	AM_CONDITIONAL(VXWORKS_6_6_DIST, false)
	AM_CONDITIONAL(VXWORKS_6_4_DIST, false)

	AC_MSG_CHECKING(for selected VxWorks distribution)
	AC_ARG_WITH(dist,
		AS_HELP_STRING(
			[--with-dist@<:@=TYPE@:>@],
			[select VxWorks distribution type, @<:@TYPE=vxworks6.4|vxworks6.6@:>@]
		),
		[
			case $withval in
				vxworks6.4)
					AM_CONDITIONAL(VXWORKS_6_4_DIST, true)
					AC_MSG_RESULT([VxWorks 6.4 distribution])
				;;
				vxworks6.6)
					AM_CONDITIONAL(VXWORKS_6_6_DIST, true)
					AC_MSG_RESULT([VxWorks 6.6 distribution])
				;;
				*)
					AC_MSG_ERROR([unrecognised distribution])
				;;
			esac
		],
		[
			AM_CONDITIONAL(VXWORKS_6_6_DIST, true)
			AC_MSG_RESULT([none, VxWorks 6.6 (default) distribution])
		]
	)
])
