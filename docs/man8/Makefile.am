MAN8_FILES  = \
	tinyproxy.txt

A2X_ARGS = \
	-d manpage \
	-f manpage

man_MANS = \
	$(MAN8_FILES:.txt=.8)

.txt.8:
if HAVE_A2X
	a2x $(A2X_ARGS) $<
else
	@echo "*** a2x (asciidoc) is required to regenerate $(@) ***"; exit 1;
endif

EXTRA_DIST = \
        $(man_MANS)

CLEANFILES = \
	$(MAN8_FILES:.txt=.xml)
