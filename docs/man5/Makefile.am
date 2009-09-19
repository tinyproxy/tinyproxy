MAN5_FILES  = \
	tinyproxy.conf.txt

A2X_ARGS = \
	-d manpage \
	-f manpage

man_MANS = \
	$(MAN5_FILES:.txt=.5)

.txt.5:
if HAVE_A2X
	a2x $(A2X_ARGS) $<
else
	@echo "*** a2x (asciidoc) is required to regenerate $(@) ***"; exit 1;
endif

EXTRA_DIST = \
        $(man_MANS)

CLEANFILES = \
	$(MAN5_FILES:.txt=.5) \
	$(MAN5_FILES:.txt=.xml)
