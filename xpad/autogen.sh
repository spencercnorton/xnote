#!/bin/sh
echo '* running gettextize...'
gettextize --force --intl --no-changelog
echo '* running aclocal...'
aclocal -I m4
echo '* running autoheader...'
autoheader --force
echo '* running automake...'
automake --foreign --add-missing --force-missing
echo '* running autoconf...'
autoconf --force
echo '* running configure...'
./configure && echo "* configure succeeded. Type 'make' to build."
