# HTML Variable Substitution Files
HTML_VAR_SUBST = HTML_VARIABLES debug.html default.html stats.html
man_MANS = tinyproxy.8

EXTRA_DIST = $(man_MANS) tinyproxy.conf \
	HTTP_ERROR_CODES RFC_INFO \
	filter-howto.txt \
	report.sh.tmpl \
	$(HTML_VAR_SUBST)

install-exec-local: report.sh

# Build the report.sh script
report.sh: $(srcdir)/report.sh.tmpl
	sed -e "s,@TINYPROXY_LOCATION@,$(DESTDIR)$(sbindir)," < $(srcdir)/report.sh.tmpl > $(top_builddir)/doc/report.sh
	chmod 755 $(top_builddir)/doc/report.sh
	@echo ""
	@echo "Report tinyproxy"
	@echo "----------------"
	@echo "Run the $(top_builddir)/doc/report.sh script to report a"
	@echo "successful installation to the authors."
	@echo ""