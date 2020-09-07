if HAVE_MANPAGE_INTEREST
MAN5_FILES  = \
	tinyproxy.conf.txt
endif

M_SECTION=5
M_NAME=TINYPROXY.CONF

man_MANS = \
	$(MAN5_FILES:.txt=.5)

.txt.5:
if HAVE_POD2MAN
	$(AM_V_GEN) $(POD2MAN) --center="Tinyproxy manual" \
	--section=$(M_SECTION) --name=$(M_NAME) --release="Version @VERSION@" \
	$< > $@
else
	@echo "*** pod2man is required to regenerate $(@) ***"; exit 1;
endif

MAINTAINERCLEANFILES = \
	$(MAN5_FILES:.txt=.5)

EXTRA_DIST = \
	$(MAN5_FILES:.txt=.5)
