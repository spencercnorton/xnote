# Process this file with autoconf to produce a configure script.

# Sets up autoconf.
AC_INIT([Xpad],[2.2],[xpad-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR(src/xpad-app.c)
AC_CONFIG_AUX_DIR(scripts)
AC_CONFIG_HEADERS([config.h:config.h.in])

# Sets up automake.
AM_INIT_AUTOMAKE([gnits dist-bzip2])
AM_BINRELOC

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_INTLTOOL([0.31], [no-xml])

# Checks for libraries.
AC_PATH_XTRA
AM_PATH_GLIB_2_0
AM_PATH_GTK_2_0(2.4.0)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T

# Sets up marshaling (copied from Evolution).
# We use AC_SUBST_FILE because AC_SUBST won't deal with newlines
XPAD_MARSHAL_RULE=$srcdir/marshal.mk
AC_SUBST_FILE(XPAD_MARSHAL_RULE)

# Sets up gettext.
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$PACKAGE", [gettext domain])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.12.1)

# Determine if we're debugging
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug=[no/yes]],[turn on debugging [default=no]]),
  enable_debug="$enableval",
  enable_debug=no)
if test "$enable_debug" = "yes"; then
  DEBUG_CFLAGS="-ansi -pedantic -Wall -Werror -g -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
else
  DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)

# Sets up output files.
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile images/Makefile po/Makefile.in autopackage/Makefile autopackage/default.apspec])
AC_OUTPUT
