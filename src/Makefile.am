# $Id: Makefile.am,v 1.8 2001-12-15 05:57:40 rjkaes Exp $
#
# 
#
# Copyright (C) 2000  Robert James Kaes (rjkaes@flarenet.com)
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#

YFLAGS = @YFLAGS@
LDFLAGS = @LDFLAGS@

bin_PROGRAMS = tinyproxy

tinyproxy_SOURCES = \
	conns.c conns.h \
	buffer.c buffer.h \
	log.c log.h \
	reqs.c reqs.h \
	sock.c sock.h \
	tinyproxy.c tinyproxy.h \
	utils.c utils.h \
	anonymous.c anonymous.h \
	stats.c stats.h \
	thread.c thread.h \
	grammar.y scanner.l \
	acl.c acl.h \
	ternary.c ternary.h \
	regexp.h

tinyproxy_LDADD = @LIBOBJS@

EXTRA_DIST = gnuregex.c gnuregex.h \
	filter.c filter.h

EXTRA_tinyproxy_SOURCES = grammar.h

scanner.c: scanner.l grammar.h
	$(LEX) $(AM_LFLAGS) $(LFLAGS) -i $< && mv $(LEX_OUTPUT_ROOT).c $@

clean:
	rm -f *.da
	rm -f gmon.out