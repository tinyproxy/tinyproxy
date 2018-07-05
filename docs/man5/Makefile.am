MAN5_FILES  = \
	tinyproxy.conf.txt

if HAVE_XMLLINT
A2X_ARGS = -d manpage -f manpage
else
A2X_ARGS = -d manpage -f manpage -L
endif

if HAVE_A2X
man_MANS = \
	$(MAN5_FILES:.txt=.5)

.txt.5:
	$(AM_V_GEN) $(A2X) $(A2X_ARGS) $<

CLEANFILES = \
	$(MAN5_FILES:.txt=.5) \
	$(MAN5_FILES:.txt=.xml)

EXTRA_DIST = \
	$(MAN5_FILES:.txt=.5)
endif
