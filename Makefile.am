SUBDIRS = \
	src \
	data \
	etc \
	docs \
	m4macros \
	tests

# tools want this on a single line
ACLOCAL_AMFLAGS = -I m4macros

AUTHORS: authors.xml authors.xsl
if HAVE_XSLTPROC
	$(AM_V_GEN) $(XSLTPROC) authors.xsl $< > $(@) || rm -f $(@)
else
	@echo "*** xsltproc is required to regenerate $(@) ***"; exit 1;
endif

validate-authors:
if HAVE_XMLLINT
	@$(XMLLINT) --noout --path $(srcdir) --valid authors.xml || \
          ( echo "*** authors.xml IS INVALID ***"; exit 1; )
endif

all-local: AUTHORS

check-local: validate-authors

EXTRA_DIST = \
	autogen.sh \
	tinyproxy-indent.sh \
	TODO

test: all
	./tests/scripts/run_tests.sh

valgrind-test: all
	./tests/scripts/run_tests_valgrind.sh
