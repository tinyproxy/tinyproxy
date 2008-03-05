#!/bin/sh
rm -f config.cache
echo "- aclocal."
aclocal
echo "- autoconf."
autoconf
echo "- autoheader."
autoheader
echo "- automake."
automake -a
exit
