# Process this file with autoconf to produce a configure script.

# Sets up autoconf.
AC_INIT([xpad],[2.0],[xpad-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR(src/xpad-app.c)
AC_CONFIG_AUX_DIR(scripts)
AC_CONFIG_HEADERS([config.h:config.h.in])

# Sets up automake.
AM_INIT_AUTOMAKE([gnits dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_INTLTOOL

# Checks for libraries.
AC_PATH_XTRA
AM_PATH_GTK_2_0(2.4.0)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T

# Sets up gettext.
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$PACKAGE", [gettext domain])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.12.1)

# Sets up output files.
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile images/Makefile po/Makefile.in])
AC_OUTPUT
