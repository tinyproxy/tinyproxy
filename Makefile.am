SUBDIRS = \
	src \
	data \
	etc \
	docs \
	m4macros \
	tests

# tools want this on a single line
ACLOCAL_AMFLAGS = -I m4macros

EXTRA_DIST = \
	autogen.sh \
	tinyproxy-indent.sh \
	TODO

test: all
	./tests/scripts/run_tests.sh

test-wait:
	TINYPROXY_TESTS_WAIT=yes $(MAKE) test

valgrind-test: all
	./tests/scripts/run_tests_valgrind.sh

valgrind-test-wait:
	TINYPROXY_TESTS_WAIT=yes $(MAKE) valgrind-test
