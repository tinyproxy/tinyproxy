
# The following files are required for the build process
CONFIG_FILES = \
  config.guess config.sub \
  install-sh missing \
  m4macros/typecheck.m4 m4macros/argenable.m4

EXTRA_DIST = $(CONFIG_FILES) TODO autogen.sh
SUBDIRS = src data etc doc packaging

install-data-local: tinyproxy-html-files

# Install the HTML files
tinyproxy-html-files:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/tinyproxy

	for file in debug default stats; do \
	  $(INSTALL) -m 644 $(srcdir)/data/templates/$$file.html $(DESTDIR)$(datadir)/tinyproxy/$$file.html.dist ; \
	  test -f $(DESTDIR)$(datadir)/tinyproxy/$$file.html || \
	    $(INSTALL) -m 644 $(srcdir)/data/templates/$$file.html $(DESTDIR)$(datadir)/tinyproxy/$$file.html ; \
	done
	$(INSTALL) -m 644 $(srcdir)/doc/HTML_VARIABLES $(DESTDIR)$(datadir)/tinyproxy/HTML_VARIABLES
