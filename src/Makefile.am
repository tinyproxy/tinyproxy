bin_PROGRAMS = tinyproxy
tinyproxy_SOURCES = config.h \
	buffer.c buffer.h \
	conns.c conns.h \
	dnscache.c dnscache.h \
	log.c log.h \
	reqs.c reqs.h \
	sock.c sock.h \
	tinyproxy.c tinyproxy.h \
	uri.c uri.h \
	utils.c utils.h \
	regexp.h

tinyproxy_LDADD = @LIBOBJS@

EXTRA_DIST = gnuregex.c gnuregex.h \
	filter.c filter.h

LDADD = @LIBOBJS
