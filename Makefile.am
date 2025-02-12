SUBDIRS = \
	src \
	data \
	etc \
	docs \
	m4macros \
	tests \
	scripts

# tools want this on a single line
ACLOCAL_AMFLAGS = -I m4macros

all-local:

dist_doc_DATA = \
	AUTHORS \
	NEWS \
	README \
	README.md

EXTRA_DIST = \
	autogen.sh \
	tinyproxy-indent.sh \
	TODO \
	VERSION

test: all
	./tests/scripts/run_tests.sh

.PHONY: shellcheck
shellcheck:
	@shellcheck `find . -name '*.sh'`

test-wait:
	TINYPROXY_TESTS_WAIT=yes $(MAKE) test

valgrind-test: all
	./tests/scripts/run_tests_valgrind.sh

valgrind-test-wait:
	TINYPROXY_TESTS_WAIT=yes $(MAKE) valgrind-test
