dnl Process this file with autoconf to produce a configure script.
AC_INIT(xpad, 1.11)
AC_CONFIG_SRCDIR(src/fio.c)
AC_CONFIG_AUX_DIR(scripts)

dnl Set up automake
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h:config.h.in)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl Checks for libraries.
AC_PATH_XTRA
AM_PATH_GTK_2_0(2.0.0)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(mkdir select socket strcspn strstr strtol)

ALL_LINGUAS="eo ro ru"
AM_GNU_GETTEXT
AC_OUTPUT(Makefile src/Makefile doc/Makefile images/Makefile intl/Makefile po/Makefile.in m4/Makefile)
