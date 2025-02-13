#!/bin/sh

# testsuite runner for tinyproxy : valgrind wrapper
#
# Copyright (C) 2009 Michael Adam <obnox@samba.org>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>.


SCRIPTS_DIR=$(dirname "$0")
TESTS_DIR=$SCRIPTS_DIR/..
TESTENV_DIR=$TESTS_DIR/env
LOG_DIR=$TESTENV_DIR/var/log

VALGRIND="valgrind --track-origins=yes --show-leak-kinds=all --tool=memcheck --leak-check=full --log-file=$LOG_DIR/valgrind.log" "$SCRIPTS_DIR"/run_tests.sh

