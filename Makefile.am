EXTRA_DIST = $(CONFIG_FILES) TODO autogen.sh
SUBDIRS = src doc packaging

install-data-local: tinyproxy-configure-file tinyproxy-html-files

# Install the configuration file if it doesn't already exist
tinyproxy-configure-file:
	$(mkinstalldirs) $(DESTDIR)@TINYPROXY_CONFIG_DIR@
	$(INSTALL) -m 600 $(srcdir)/doc/tinyproxy.conf \
		$(DESTDIR)@TINYPROXY_CONFIG_DIR@/@TINYPROXY_CONFIG_FILE@-dist
	test -f $(DESTDIR)@TINYPROXY_CONFIG_DIR@/@TINYPROXY_CONFIG_FILE@ \
		|| $(INSTALL) -m 600 $(srcdir)/doc/tinyproxy.conf \
			$(DESTDIR)@TINYPROXY_CONFIG_DIR@/@TINYPROXY_CONFIG_FILE@
	@echo ""
	@echo "A configuration file has been copied to:"
	@echo ""
	@echo "@TINYPROXY_CONFIG_DIR@/@TINYPROXY_CONFIG_FILE@-dist"
	@echo ""
	@echo "You will need to rename this file to:"
	@echo ""
	@echo "@TINYPROXY_CONFIG_DIR@/@TINYPROXY_CONFIG_FILE@"
	@echo ""
	@echo "and modify the values to suit your local system."
	@echo ""
	@echo "All the configuration directives are commented in the file, so"
	@echo "you should not have any problems configuring your system."
	@echo ""

# Install the HTML files
tinyproxy-html-files:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/tinyproxy

	for file in debug default stats; do \
	  $(INSTALL) -m 644 $(srcdir)/doc/$$file.html $(DESTDIR)$(datadir)/tinyproxy/$$file.html.dist ; \
	  test -f $(DESTDIR)$(datadir)/tinyproxy/$$file.html || \
	    $(INSTALL) -m 644 $(srcdir)/doc/$$file.html $(DESTDIR)$(datadir)/tinyproxy/$$file.html ; \
	done
	$(INSTALL) -m 644 $(srcdir)/doc/HTML_VARIABLES $(DESTDIR)$(datadir)/tinyproxy/HTML_VARIABLES
