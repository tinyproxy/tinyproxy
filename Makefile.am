EXTRA_DIST = TODO
SUBDIRS = src doc

install-data-local:
	mkdir -p @TINYPROXY_CONFIG_DIR@
	$(INSTALL) doc/tinyproxy.conf @TINYPROXY_CONFIG@-dist
	test ! -f @TINYPROXY_CONFIG@ && $(INSTALL) doc/tinyproxy.conf @TINYPROXY_CONFIG@
	echo ""
	echo "A configuration file has been copied to:"
	echo "@TINYPROXY_CONFIG@-dist"
	echo "You will need to rename this file to:"
	echo "@TINYPROXY_CONFIG@"
	echo "and modify the values to suit your local system."
	echo ""
	echo "All the configuration directives are commented in the file, so"
	echo "you should not have any problems configuring your system."
	echo ""