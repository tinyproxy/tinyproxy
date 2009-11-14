MAN5_FILES  = \
	tinyproxy.conf.txt

A2X_ARGS = \
	-d manpage \
	-f manpage

man_MANS = \
	$(MAN5_FILES:.txt=.5)

.txt.5:
if HAVE_A2X
	$(AM_V_GEN) $(A2X) $(A2X_ARGS) $<
else
	@echo "*** a2x (asciidoc) is required to regenerate $(@) ***"; exit 1;
endif

CLEANFILES = \
	$(MAN5_FILES:.txt=.5) \
	$(MAN5_FILES:.txt=.xml)
