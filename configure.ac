#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([GNU APL], [1.2], [bug-apl@gnu.org])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

LT_INIT([dlopen])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_GNU_SOURCE
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

# Checks for header files.
AC_CHECK_HEADERS([	    \
	curses.h term.h	    \
	execinfo.h	    \
	fcntl.h		    \
	inttypes.h	    \
	stdint.h	    \
	stdlib.h	    \
        string.h	    \
	strings.h	    \
	unistd.h	    \
	utmpx.h		    ])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([floor memset munmap pow sqrt strchr strerror])

# Checks for libraries.
AC_CHECK_LIB([lapack],    [dgelsy_])
AC_CHECK_LIB([blas],      [dcopy_])
#AC_CHECK_LIB([gfortran], [_gfortran_transpose])
AC_CHECK_LIB([execinfo],  [backtrace])
AC_CHECK_LIB([intl],      [libintl_gettext])
AC_CHECK_LIB([m],         [acosh])
AC_CHECK_LIB([rt],        [shm_open])
AC_CHECK_LIB([pthread],   [sem_init])
AC_CHECK_LIB([ncurses],   [setupterm])
AC_CHECK_LIB([dl],        [dlopen])

# enable readline support if libreadline is present and the user
# has not disabled it.
#
AC_ARG_WITH( [readline],
             [AS_HELP_STRING([--with-readline],
             [support fancy command line editing @<:@default=check@:>@])],
             [],
             [with_readline=check])

LIBREADLINE=
    AS_IF(
	[test "x$with_readline" != xno],
	[AC_CHECK_LIB(
	    [readline],
	    [readline],
            [AC_SUBST([LIBREADLINE], ["-lreadline -lncurses"])
             AC_DEFINE([HAVE_LIBREADLINE],
                      [1],
                      [Define if you have libreadline])
	    ],
	    [ if test "x$with_readline" != xcheck; then
		AC_MSG_FAILURE(
		    [--with-readline was given, but test for readline failed])
	      fi
	    ],
	    -lncurses)
	])

AC_CONFIG_FILES([Makefile
                 debian/Makefile
                 debian/source/Makefile
		 po/Makefile.in po/Makefile
                 rpm/Makefile
		 rpm/apl.spec
                 src/Makefile
                 src/APs/Makefile
                 src/emacs_mode/Makefile
                 src/native/Makefile
                 src/testcases/Makefile
		 support-files/Makefile
                 workspaces/Makefile
                 wslib1/Makefile
                 wslib2/Makefile])

# APL: max. rank ?
AC_ARG_VAR(MAX_RANK_WANTED,
   [ the max. Rank of APL values (see README-2-configure) default: 8) ])
if test "x$MAX_RANK_WANTED" = "x"; then MAX_RANK_WANTED=8; fi
AC_DEFINE_UNQUOTED([MAX_RANK_WANTED], [$MAX_RANK_WANTED],
                   [max. rank of APL values ])


# APL: CHECK() macro desired ?
AC_ARG_VAR(VALUE_CHECK_WANTED,
   [ enable CHECK() macro (see README-2-configure) default: no ])
if test "x$VALUE_CHECK_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VALUE_CHECK_WANTED], [yes],
                   [ define to enable CHECK macro ])
fi


# APL: ADD_EVENT() macro and friends desired ?
AC_ARG_VAR(VALUE_HISTORY_WANTED,
   [ enable CHECK() macro (see README-2-configure) default: no ])
if test "x$VALUE_HISTORY_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VALUE_HISTORY_WANTED], [yes],
                   [ define to enable CHECK macro ])
fi


# APL: gprof profiling ?
AC_ARG_VAR(GPROF_WANTED,
   [ enable gprof (-pg) (see README-2-configure) default: no ])
if test "x$GPROF_WANTED" = "xyes"; then
   GPROF_WANTED=-pg
else
   GPROF_WANTED=
fi

AC_DEFINE_UNQUOTED([GPROF_WANTED], [-pg],
                   [ define to enable gprof profiling ])


# APL: dynamic logging desired ?
AC_ARG_VAR(DYNAMIC_LOG_WANTED,
   [ enable dynamic (see README-2-configure) default: no ])
if test "x$DYNAMIC_LOG_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([DYNAMIC_LOG_WANTED], [yes],
                   [ define to enable dynamic logging ])
fi


# APL: print semaphore desired ?
AC_ARG_VAR(PRINT_SEMA_WANTED,
   [ enable print semaphore (see README-2-configure) default: no ])
if test "x$PRINT_SEMA_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([PRINT_SEMA_WANTED], [yes],
                   [ define to enable the print semaphore ])
fi


# APL: tracing of value flags (produces lots of messages) desired ?
AC_ARG_VAR(VF_TRACING_WANTED,
   [ enable tracing of value flags (see README-2-configure) default: no ])
if test "x$VF_TRACING_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VF_TRACING_WANTED], [yes],
                   [ define to enable tracing of value flags ])
fi


# APL: desired assert level ?
AC_ARG_VAR(ASSERT_LEVEL_WANTED,
   [ Assert level (see README-2-configure) default: 1) ])
if test "x$ASSERT_LEVEL_WANTED" = "x"; then ASSERT_LEVEL_WANTED=1; fi
AC_DEFINE_UNQUOTED([ASSERT_LEVEL_WANTED], [$ASSERT_LEVEL_WANTED],
                   [assert level])


# APL: desired cellcount of short values ?
AC_ARG_VAR(SHORT_VALUE_LENGTH_WANTED,
    [ Cellcount of short values (see README-2-configure) default: 12) ])
if test "x$SHORT_VALUE_LENGTH_WANTED" = "x"; then SHORT_VALUE_LENGTH_WANTED=12; fi
AC_DEFINE_UNQUOTED([SHORT_VALUE_LENGTH_WANTED], [$SHORT_VALUE_LENGTH_WANTED],
                   [short value cellcount])


# APL: desired colors for CERR, CIN, and COUT ?
AC_ARG_VAR(CERR_COLOR_WANTED,   [ obsolete option ])
AC_ARG_VAR(CIN_COLOR_WANTED,    [ obsolete option ])
AC_ARG_VAR(COUT_COLOR_WANTED,   [ obsolete option ])
AC_ARG_VAR(RESET_COLORS_WANTED, [ obsolete option ])
AC_ARG_VAR(CLEAR_EOL_WANTED,    [ obsolete option ])


# check if we are on i686 (to fix a strange rpmbuild problem
AM_CONDITIONAL([ON_i686], [test `uname -m` = i686])

AC_OUTPUT
