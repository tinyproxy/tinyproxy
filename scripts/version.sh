#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "${0}")" && pwd)"
GIT_DIR="${SCRIPT_DIR}/../.git"

if test -d "${GIT_DIR}" ; then
	if type git >/dev/null 2>&1 ; then
		gitstr=$(git describe --match '[0-9]*.[0-9]*.*' 2>/dev/null)
		if test "x$?" != x0 ; then
			sed 's/$/-git/' < VERSION
		else
			printf "%s\n" "$gitstr" | sed -e 's/-g/-git-/'
		fi
	else
		sed 's/$/-git/' < VERSION
	fi
else
	cat VERSION
fi
