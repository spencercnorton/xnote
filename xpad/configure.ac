# Process this file with autoconf to produce a configure script.

# Sets up autoconf.
AC_INIT([Xpad],[2.8],[xpad-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR(src/xpad-app.c)
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
AM_PATH_GTK_2_0(2.6.0,,AC_MSG_ERROR([
  *** GTK+ 2.6 is required to build Xpad; please make sure you have the GTK+
  *** development headers installed. The latest version of GTK+ is
  *** always available at http://gtk.org/.]))

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Sets up marshaling (copied from Evolution).
# We use AC_SUBST_FILE because AC_SUBST won't deal with newlines
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
XPAD_MARSHAL_RULE=$srcdir/marshal.mk
AC_SUBST_FILE(XPAD_MARSHAL_RULE)

# Sets up gettext.
GETTEXT_PACKAGE="$PACKAGE"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext domain])
AM_GLIB_GNU_GETTEXT

# Determines if we're debugging.
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug=[no/yes]],[turn on debugging [default=no]]),
  enable_debug="$enableval",
  enable_debug=no)
if test "$enable_debug" = "yes"; then
  DEBUG_CFLAGS="-ansi -Wall -Werror -g -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGTK_MULTIHEAD_SAFE"
else
  DEBUG_CFLAGS=""
fi
AC_SUBST(DEBUG_CFLAGS)

# Sets up output files.
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile images/Makefile po/Makefile.in autopackage/Makefile autopackage/default.apspec])
AC_OUTPUT
