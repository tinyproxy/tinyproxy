# $Id: Makefile.am,v 1.14 2002-05-26 18:56:43 rjkaes Exp $
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

sbin_PROGRAMS = tinyproxy

tinyproxy_SOURCES = \
	acl.c acl.h \
	anonymous.c anonymous.h \
	buffer.c buffer.h \
	child.c child.h \
	common.h \
	conns.c conns.h \
	daemon.c daemon.h \
	hashmap.c hashmap.h \
	heap.c heap.h \
	log.c log.h \
	network.c network.h \
	reqs.c reqs.h \
	sock.c sock.h \
	stats.c stats.h \
	text.c text.h \
	tinyproxy.c tinyproxy.h \
	utils.c utils.h \
	vector.c vector.h \
	grammar.y scanner.l \
	regexp.h

EXTRA_DIST = gnuregex.c gnuregex.h
EXTRA_tinyproxy_SOURCES = filter.c filter.h grammar.h
tinyproxy_DEPENDENCIES = @ADDITIONAL_OBJECTS@
tinyproxy_LDADD = @ADDITIONAL_OBJECTS@

scanner.c: scanner.l grammar.h
	$(LEX) $(AM_LFLAGS) $(LFLAGS) -i $< && mv $(LEX_OUTPUT_ROOT).c $@

clean:
	rm -f *.da
	rm -f gmon.out
