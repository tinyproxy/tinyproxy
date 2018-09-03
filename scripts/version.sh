#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "${0}")" && pwd)"
GIT_DIR="${SCRIPT_DIR}/../.git"

if test -d "${GIT_DIR}" ; then
	if type git >/dev/null 2>&1 ; then
		git describe --match '[0-9]*.[0-9]*.[0-9]*' 2>/dev/null \
			| sed -e 's/-/-git-/'
	else
		sed 's/$/-git/' < VERSION
	fi
else
	cat VERSION
fi
