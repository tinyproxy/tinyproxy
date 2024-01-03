if HAVE_MANPAGE_INTEREST
MAN8_FILES  = \
	tinyproxy.txt
endif

M_SECTION=8
M_NAME=TINYPROXY

man_MANS = \
	$(MAN8_FILES:.txt=.8)

edit = sed \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@runstatedir[@]|$(runstatedir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@TINYPROXY_STATHOST[@]|$(TINYPROXY_STATHOST)|g'

tinyproxy.txt: $(top_srcdir)/docs/man8/tinyproxy.txt.in Makefile
	@rm -f $@ $@.tmp
	$(AM_V_GEN) $(edit) $(top_srcdir)/docs/man8/$@.in > $@.tmp
	@mv $@.tmp $@

.txt.8:
if HAVE_POD2MAN
	$(AM_V_GEN) $(POD2MAN) --center="Tinyproxy manual" \
	--section=$(M_SECTION) --name=$(M_NAME) --release="Version @VERSION@" \
	$< > $@
else
	@echo "*** pod2man is required to regenerate $(@) ***"; exit 1;
endif

MAINTAINERCLEANFILES = \
	$(MAN8_FILES:.txt=.8)

EXTRA_DIST = \
	$(MAN8_FILES:.txt=.8)
