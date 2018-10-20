#!/bin/sh

checkInstalled () {
	which "$1" || { echo "$1 not installed"; exit 0; }
}

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
ORIGDIR=`pwd`

set -x

cd $srcdir

checkInstalled "automake"
checkInstalled "autoconf"

aclocal -I m4macros \
  && autoheader \
  && automake --gnu --add-missing \
  && autoconf

cd $ORIGDIR

set -

echo $srcdir/configure "$@"
$srcdir/configure "$@"
RC=$?
if test $RC -ne 0; then
  echo
  echo "Configure failed or did not finish!"
  exit $RC
fi

echo
echo "Now type 'make' to compile Tinyproxy."
