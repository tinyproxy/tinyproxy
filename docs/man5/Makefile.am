if HAVE_MANPAGE_INTEREST
MAN5_FILES  = \
	tinyproxy.conf.txt
endif

if HAVE_XMLLINT
A2X_ARGS = -d manpage -f manpage
else
A2X_ARGS = -d manpage -f manpage -L
endif

ASCIIDOCTOR_ARGS = -b manpage

man_MANS = \
	$(MAN5_FILES:.txt=.5)

.txt.5:
if HAVE_A2X
	$(AM_V_GEN) $(A2X) $(A2X_ARGS) $<
else
if HAVE_ASCIIDOCTOR
	$(AM_V_GEN) $(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) $<
else
	@echo "*** a2x (asciidoc) or asciidoctor is required to regenerate $(@) ***"; exit 1;
endif
endif

MAINTAINERCLEANFILES = \
	$(MAN5_FILES:.txt=.5)

CLEANFILES = \
	$(MAN5_FILES:.txt=.xml)

EXTRA_DIST = \
	$(MAN5_FILES:.txt=.5)
