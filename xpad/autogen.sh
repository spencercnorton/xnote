#!/bin/sh
export ACLOCAL=`which aclocal-1.9`
export AUTOMAKE=`which automake-1.9`
autoreconf --force --install --symlink
glib-gettextize --force
intltoolize --force --automake
