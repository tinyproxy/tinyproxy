# $Id: Makefile.am,v 1.23 2005-08-16 04:03:19 rjkaes Exp $
#
# Copyright (C) 2000  Robert James Kaes (rjkaes@users.sourceforge.net)
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

LDFLAGS = @LDFLAGS@

sbin_PROGRAMS = tinyproxy

tinyproxy_SOURCES = \
	acl.c acl.h \
	anonymous.c anonymous.h \
	buffer.c buffer.h \
	child.c child.h \
	common.h \
	conffile.c conffile.h \
	conns.c conns.h \
	daemon.c daemon.h \
	hashmap.c hashmap.h \
	heap.c heap.h \
	htmlerror.c htmlerror.h \
	http-message.c http-message.h \
	log.c log.h \
	network.c network.h \
	reqs.c reqs.h \
	sock.c sock.h \
	stats.c stats.h \
	text.c text.h \
	tinyproxy.c tinyproxy.h \
	utils.c utils.h \
	vector.c vector.h

EXTRA_tinyproxy_SOURCES = filter.c filter.h \
	reverse-proxy.c reverse-proxy.h
tinyproxy_DEPENDENCIES = @ADDITIONAL_OBJECTS@
tinyproxy_LDADD = @ADDITIONAL_OBJECTS@

