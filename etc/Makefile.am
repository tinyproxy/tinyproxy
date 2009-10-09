sysconf_DATA = \
	tinyproxy.conf

EXTRA_DIST = \
	$(sysconf_DATA)

edit = sed \
	-e 's|@bindir[@]|$(bindir)|g' \
	-e 's|@datadir[@]|$(datadir)|g' \
	-e 's|@datarootdir[@]|$(datarootdir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
	-e 's|@prefix[@]|$(prefix)|g'

tinyproxy.conf: tinyproxy.conf.in
	rm -f $@ $@.tmp
	$(edit) $@.in >$@.tmp
	mv $@.tmp $@

