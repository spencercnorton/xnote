dnl Process this file with autoconf to produce a configure script.
AC_INIT([xpad],[1.11],[xpad-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR(src/fio.c)
AC_CONFIG_AUX_DIR(scripts)
AC_CONFIG_HEADERS([config.h:config.h.in])

dnl Set up automake
AM_INIT_AUTOMAKE([dist-bzip2 std-options])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_INTLTOOL

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

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.12.1)
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile images/Makefile po/Makefile.in])
AC_OUTPUT
