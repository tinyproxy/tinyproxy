#!/bin/sh

srcdir=$(dirname "$0")
test -z "$srcdir" && srcdir=.
ORIGDIR=$(pwd)

set -x

cd "$srcdir" || {
    echo "error changing to dir '$srcdir'"
    exit
}

aclocal -I m4macros \
  && autoheader \
  && automake --gnu --add-missing \
  && autoconf

cd "$ORIGDIR" || {
    echo "error changing to idir '$ORIGDIR'"
    exit

}

set -

echo "$srcdir"/configure "$@"
"$srcdir"/configure "$@"
RC=$?
if test $RC -ne 0; then
  echo
  echo "Configure failed or did not finish!"
  exit $RC
fi

echo
echo "Now type 'make' to compile Tinyproxy."
