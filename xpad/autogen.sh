#!/bin/sh
autoreconf --force --install --symlink
glib-gettextize --force
intltoolize --force --automake
