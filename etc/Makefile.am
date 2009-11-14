sysconf_DATA = \
	tinyproxy.conf

EXTRA_DIST = \
	tinyproxy.conf.in

edit = sed \
	-e 's|@bindir[@]|$(bindir)|g' \
	-e 's|@datadir[@]|$(datadir)|g' \
	-e 's|@datarootdir[@]|$(datarootdir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
	-e 's|@prefix[@]|$(prefix)|g' \
	-e 's|@TINYPROXY_STATHOST[@]|$(TINYPROXY_STATHOST)|g'

tinyproxy.conf: $(top_srcdir)/etc/tinyproxy.conf.in Makefile
	@rm -f $@ $@.tmp
	$(AM_V_GEN) $(edit) $(top_srcdir)/etc/$@.in > $@.tmp
	@mv $@.tmp $@

CLEANFILES = \
	tinyproxy.conf
