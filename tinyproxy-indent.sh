#!/bin/sh

# This is a script to mostly indent C code to the Tinyproxy coding
# style. It needs GNU indent 2.2.10 or above. In a nutshell, Tinyproxy
# uses the K&R style, tab size 8, spaces instead of tabs, wrap at column
# 80, space before brackets.

indent -npro -kr -i8 -ts8 -sob -l80 -ss -cs -cp1 -bs -nlps -nprs -pcs \
    -saf -sai -saw -sc -cdw -ce -nut -il0 "$@"
