#!/bin/sh
# -*- sh -*-
#
# Make the Autotools scripts after checking out the source code from CVS.
# This script was taken from the Autotool Book.  I wonder if autoreconf
# can now be used...
#

set -x
aclocal \
  && autoheader \
  && automake --gnu --add-missing \
  && autoconf
