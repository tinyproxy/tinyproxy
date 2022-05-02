# tinyproxy - A fast light-weight HTTP proxy
# Copyright (C) 2000 Robert James Kaes <rjkaes@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

pkgsysconfdir = $(sysconfdir)/$(PACKAGE)

bin_PROGRAMS = tinyproxy

AM_CPPFLAGS = \
	-DSYSCONFDIR=\"${pkgsysconfdir}\" \
	-DLOCALSTATEDIR=\"${localstatedir}\"

tinyproxy_SOURCES = \
	hostspec.c hostspec.h \
	acl.c acl.h \
	anonymous.c anonymous.h \
	buffer.c buffer.h \
	child.c child.h \
	common.h \
	conf-tokens.c conf-tokens.h \
	conf.c conf.h \
	conns.c conns.h \
	daemon.c daemon.h \
	heap.c heap.h \
	html-error.c html-error.h \
	http-message.c http-message.h \
	log.c log.h \
	network.c network.h \
	reqs.c reqs.h \
	sock.c sock.h \
	stats.c stats.h \
	text.c text.h \
	main.c main.h \
	utils.c utils.h \
	upstream.c upstream.h \
	basicauth.c basicauth.h \
	base64.c base64.h \
	sblist.c sblist.h \
	hsearch.c hsearch.h \
	pseudomap.c pseudomap.h \
	loop.c loop.h \
	mypoll.c mypoll.h \
	connect-ports.c connect-ports.h

EXTRA_tinyproxy_SOURCES = filter.c filter.h \
	reverse-proxy.c reverse-proxy.h \
	transparent-proxy.c transparent-proxy.h
tinyproxy_DEPENDENCIES = @ADDITIONAL_OBJECTS@
tinyproxy_LDADD = @ADDITIONAL_OBJECTS@ -lpthread

if HAVE_GPERF
conf-tokens.c: conf-tokens-gperf.inc
conf-tokens-gperf.inc: conf-tokens.gperf
	$(GPERF) $< > $@
endif

EXTRA_DIST = conf-tokens.gperf conf-tokens-gperf.inc

