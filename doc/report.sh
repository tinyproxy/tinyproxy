#!/bin/sh

(echo "date: "
date
echo "uname: "
uname -a
echo "ps: "
ps -auxw | grep [t]inyproxy -
echo "ver: "
if [ -x /usr/local/bin/tinyproxy ]; then
   /usr/local/bin/tinyproxy -v
else
   echo no ver available.
fi;) 2>&1 | mail -s 'tinyproxy install report' rjkaes@users.sourceforge.net
