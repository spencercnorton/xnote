#!/bin/sh
touch stamp-h
echo '* running aclocal...'
aclocal
#echo '* running libtoolize...'
#libtoolize --force
#echo '* linking config/ltmain.sh to ./ltmain.sh...'
#cd config && ln -sf ../ltmain.sh && cd ..
echo '* running autoheader...'
autoheader --force
echo '* running automake...'
automake --foreign --add-missing --force-missing
echo '* running gettextize...'
gettextize --force --intl --no-changelog
echo '* re-running aclocal...'
aclocal -I m4
echo '* running autoconf...'
autoconf --force
echo '* running configure...'
./configure && echo "* configure succeeded. Type 'make' to build."
