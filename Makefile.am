EXTRA_DIST = TODO
SUBDIRS = src doc

install-data-local:
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