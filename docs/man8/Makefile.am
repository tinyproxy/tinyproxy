MAN8_FILES  = \
	tinyproxy.txt

A2X_ARGS = \
	-d manpage \
	-f manpage

man_MANS = \
	$(MAN8_FILES:.txt=.8)

.txt.8:
if HAVE_A2X
	$(AM_V_GEN) $(A2X) $(A2X_ARGS) $<
else
	@echo "*** a2x (asciidoc) is required to regenerate $(@) ***"; exit 1;
endif

CLEANFILES = \
	$(MAN8_FILES:.txt=.8) \
	$(MAN8_FILES:.txt=.xml)
