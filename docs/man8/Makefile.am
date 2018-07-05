MAN8_FILES = \
	tinyproxy.txt

if HAVE_XMLLINT
A2X_ARGS = -d manpage -f manpage
else
A2X_ARGS = -d manpage -f manpage -L
endif

if HAVE_A2X
man_MANS = \
	$(MAN8_FILES:.txt=.8)

.txt.8:
	$(AM_V_GEN) $(A2X) $(A2X_ARGS) $<

CLEANFILES = \
	$(MAN8_FILES:.txt=.8) \
	$(MAN8_FILES:.txt=.xml)

EXTRA_DIST= \
	$(MAN8_FILES:.txt=.8)
endif
