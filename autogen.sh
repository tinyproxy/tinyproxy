#!/bin/sh
# -*- sh -*-
#
# Make the Autotools scripts after checking out the source code from CVS.
# This script was taken from the Autotool Book.  I wonder if autoreconf
# can now be used...
#

test -d config || mkdir config
set -x
aclocal -I config \
  && autoheader \
  && automake --gnu --add-missing \
  && autoconf
