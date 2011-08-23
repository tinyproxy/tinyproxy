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

sbin_PROGRAMS = tinyproxy

AM_CPPFLAGS = \
	-DSYSCONFDIR=\"${pkgsysconfdir}\" \
	-DLOCALSTATEDIR=\"${localstatedir}\"

tinyproxy_SOURCES = \
	acl.c acl.h \
	anonymous.c anonymous.h \
	authors.c authors.h \
	buffer.c buffer.h \
	child.c child.h \
	common.h \
	conf.c conf.h \
	conns.c conns.h \
	daemon.c daemon.h \
	hashmap.c hashmap.h \
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
	vector.c vector.h \
	upstream.c upstream.h \
	connect-ports.c connect-ports.h

EXTRA_tinyproxy_SOURCES = filter.c filter.h \
	reverse-proxy.c reverse-proxy.h \
	transparent-proxy.c transparent-proxy.h
tinyproxy_DEPENDENCIES = @ADDITIONAL_OBJECTS@
tinyproxy_LDADD = @ADDITIONAL_OBJECTS@

EXTRA_DIST = \
	authors.xsl

authors.c: $(top_srcdir)/authors.xml $(srcdir)/authors.xsl
if HAVE_XSLTPROC
	$(AM_V_GEN) $(XSLTPROC) $(srcdir)/authors.xsl $< > $(@) || rm -f $(@)
else
        @echo "*** xsltproc is required to regenerate $(@) ***"; exit 1;
endif

BUILT_SOURCES = \
	authors.c
