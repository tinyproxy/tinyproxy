#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "${0}")" && pwd)"
BASE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
AUTHORS_FILE="${BASE_DIR}/AUTHORS"

type git > /dev/null || exit
test -d "${BASE_DIR}/.git" || exit

git log --all --format='%aN' | sort -u > "${AUTHORS_FILE}"
