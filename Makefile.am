SUBDIRS = \
	src \
	data \
	etc \
	docs \
	m4macros \
	packaging \
	tests

# tools want this on a single line
ACLOCAL_AMFLAGS = -I m4macros

EXTRA_DIST = \
	autogen.sh \
	tinyproxy-indent.sh \
	TODO

test: all
	./tests/scripts/run_tests.sh

valgrind-test: all
	./tests/scripts/run_tests_valgrind.sh
