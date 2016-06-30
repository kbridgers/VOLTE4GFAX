dnl BOARD_INCL_CHECK([DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED])
dnl ----------------------------------------------------------
dnl
dnl Checks for drv_board
dnl specify --with-board-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([BOARD_INCL_CHECK],
[
	WITH_DRV_INCL_CHECK(
			[board],
			[BOARD driver],
			[drv_board drv_board-* voice_board_drv],
			[drv_board_io.h],
			[$1],
			[
				AC_SUBST([BOARD_INCL_PATH],[$cached_with_board_incl])
				ifelse([$2],,[:],[$2])
			],
			[ifelse([$3],,[:],[$3])]
		)
])dnl

