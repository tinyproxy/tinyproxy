# Process this file with autoconf to produce a configure script.
# Portions of this file were adapted from GIMP.

AC_PREREQ(2.54)

m4_define([tinyproxy_version], esyscmd(sh scripts/version.sh | tr -d '\n'))

AC_INIT([Tinyproxy], [tinyproxy_version],
	[https://tinyproxy.github.io/],
	[tinyproxy])

tpv=tinyproxy_version
if test "x$tpv" = "x" ; then
AC_MSG_ERROR([got empty result from version script!])
fi

AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([dist-bzip2 dist-xz])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([m4macros])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl Temporarily defined here until we get tinyproxy-version.h
AC_DEFINE(TINYPROXY_VERSION, "tinyproxy_version", [Tinyproxy version number])


dnl Check if we're compiling on a weird platform :)
AC_USE_SYSTEM_EXTENSIONS


dnl Set the domain name for find the statistics of tinyproxy
AH_TEMPLATE([TINYPROXY_STATHOST],
	[This controls remote proxy stats display.])
AC_ARG_WITH(stathost,
            [AC_HELP_STRING([--with-stathost=HOST], [Default status host])],
            [AC_DEFINE_UNQUOTED(TINYPROXY_STATHOST, "$withval")
	     TINYPROXY_STATHOST="$withval"],
	    [AC_DEFINE_UNQUOTED(TINYPROXY_STATHOST, "tinyproxy.stats")
	     TINYPROXY_STATHOST="tinyproxy.stats"])

AC_SUBST(TINYPROXY_STATHOST)

dnl Add compiler-specific optimization flags
TP_ARG_ENABLE(debug,
              [Enable debugging support code and methods (default is NO)],
              no)

dnl Check to see if the XTinyproxy header is to be included
AH_TEMPLATE([XTINYPROXY_ENABLE],
	    [Define if you want to have the peer's IP address included in a XTinyproxy header sent to the server.])
TP_ARG_ENABLE(xtinyproxy,
              [Include the X-Tinyproxy header (default is YES)],
              yes)

if test x"$xtinyproxy_enabled" = x"yes"; then
    AC_DEFINE(XTINYPROXY_ENABLE)
fi

dnl Include filtering for domain/URLs
AH_TEMPLATE([FILTER_ENABLE],
	    [Defined if you would like filtering code included.])
TP_ARG_ENABLE(filter,
              [Enable filtering of domains/URLS (default is YES)],
              yes)

if test x"$filter_enabled" = x"yes"; then
    ADDITIONAL_OBJECTS="$ADDITIONAL_OBJECTS filter.o"
    AC_DEFINE(FILTER_ENABLE)
fi

dnl Include support for upstream proxies?
AH_TEMPLATE([UPSTREAM_SUPPORT],
            [Include support for connecting to an upstream proxy.])
TP_ARG_ENABLE(upstream,
              [Enable upstream proxying (default is YES)],
              yes)
if test x"$upstream_enabled" = x"yes"; then
    AC_DEFINE(UPSTREAM_SUPPORT)
fi

dnl Include support for reverse proxy?
AH_TEMPLATE([REVERSE_SUPPORT],
            [Include support for reverse proxy.])
TP_ARG_ENABLE(reverse,
              [Enable reverse proxying (default is YES)],
              yes)
if test x"$reverse_enabled" = x"yes"; then
    ADDITIONAL_OBJECTS="$ADDITIONAL_OBJECTS reverse-proxy.o"
    AC_DEFINE(REVERSE_SUPPORT)
fi

dnl Include the transparent proxy support
AH_TEMPLATE([TRANSPARENT_PROXY],
	    [Include support for using tinyproxy as a transparent proxy.])
TP_ARG_ENABLE(transparent,
              [Enable transparent proxying code (default is YES)],
              yes)
if test x"$transparent_enabled" = x"yes"; then
   ADDITIONAL_OBJECTS="$ADDITIONAL_OBJECTS transparent-proxy.o"
   AC_DEFINE(TRANSPARENT_PROXY)
fi

dnl Let user decide whether he wants support for manpages
dnl Which require either pod2man or a tarball release
AH_TEMPLATE([MANPAGE_SUPPORT],
	    [Build manpages with pod2man if they are missing from the distribution.])
TP_ARG_ENABLE(manpage_support,
              [Enable support for building manpages (default is YES)],
              yes)
AM_CONDITIONAL(HAVE_MANPAGE_INTEREST, test x"$manpage_support_enabled" = x"yes")

# This is required to build test programs below
AC_PROG_CC

dnl
dnl Checks for libraries
dnl
AC_CHECK_LIB(socket, socket, , [AC_CHECK_LIB(socket, htonl)])

dnl Some systems (OpenServer 5) dislike -lsocket -lnsl, so we try to
dnl avoid -lnsl checks, if we already have the functions which are
dnl usually in libnsl

unset ac_cv_func_yp_get_default_domain
AC_CHECK_FUNC(yp_get_default_domain,
              tinyproxy_no_nsl_checks=yes,
              tinyproxy_no_nsl_checks=no)
unset ac_cv_func_yp_get_default_domain

if test x"$tinyproxy_no_nsl_checks" != x"yes"; then
    AC_CHECK_LIB(nsl, gethostname, , [AC_CHECK_LIB(nsl, gethostbyaddr)])
fi

AC_CHECK_LIB(resolv, inet_aton)

dnl
dnl Checks for headers
dnl
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([sys/ioctl.h alloca.h memory.h malloc.h sysexits.h \
		  values.h poll.h])

dnl Checks for libary functions
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK

AC_CHECK_FUNCS([strlcpy strlcat setgroups])

dnl Enable extra warnings
DESIRED_FLAGS="-fdiagnostics-show-option -Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations -Wfloat-equal -Wundef -Wformat=2 -Wlogical-op -Wmissing-include-dirs -Wformat-nonliteral -Wold-style-definition -Wpointer-arith -Waggregate-return -Winit-self -Wpacked --std=c89 -ansi -Wno-overlength-strings -Wno-long-long -Wno-overlength-strings -Wdeclaration-after-statement -Wredundant-decls -Wmissing-noreturn -Wshadow -Wendif-labels -Wcast-qual -Wcast-align -Wwrite-strings -Wp,-D_FORTIFY_SOURCE=2 -fno-common"

if test -n "${MAINTAINER_MODE_FALSE}"; then
   DESIRED_FLAGS="-Werror $DESIRED_FLAGS"
fi

all_desired_work=false
AS_COMPILER_FLAG([$DESIRED_FLAGS], [all_desired_work=true])
if $all_desired_work ; then
    CFLAGS="$CFLAGS $DESIRED_FLAGS"
else
for flag in $DESIRED_FLAGS; do
  AS_COMPILER_FLAG([$flag], [CFLAGS="$CFLAGS $flag"])
done
fi

dnl Disable debugging if it's not specified
if test x"$debug_enabled" != x"yes" ; then
    CFLAGS="-DNDEBUG $CFLAGS"
fi

AS_ECHO_N(["checking to see if linker understands -z,defs... "])
LDFLAGS_OLD="-Wl $LDFLAGS"
LDFLAGS="-Wl,-z,defs $LDFLAGS"
AC_LINK_IFELSE([AC_LANG_PROGRAM()],
    AS_ECHO("yes"),
    AS_ECHO("no"); LDFLAGS="$LDFLAGS_OLD")

dnl
dnl Make sure we can actually handle the "--with-*" and "--enable-*" stuff.
dnl

dnl
dnl Substitute the variables into the various Makefiles
dnl
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LIBS)
AC_SUBST(ADDITIONAL_OBJECTS)

if test x"$manpage_support_enabled" = x"yes"; then
AC_PATH_PROG(POD2MAN, pod2man, no)

if test "x$POD2MAN" = "xno" && \
 ! test -e docs/man5/tinyproxy.conf.5 -a -e docs/man8/tinyproxy.8 ; then
AC_MSG_ERROR([
  manpage generation requested, but neither pod2man
  nor pre-generated manpages found.
  Use --disable-manpage-support if you want to compile anyway.])
fi
fi #manpage_support_enabled

AM_CONDITIONAL(HAVE_POD2MAN, test "x$POD2MAN" != "x" -a "x$POD2MAN" != "xno")

AC_PATH_PROG(GPERF, gperf, no)
AM_CONDITIONAL(HAVE_GPERF, test "x$GPERF" != "x" -a "x$GPERF" != "xno")
AH_TEMPLATE([HAVE_GPERF],
	    [Whether you have gperf installed for faster config parsing.])

if test "x$GPERF" != "x" -a "x$GPERF" != "xno" ; then
  AS_ECHO_N(["checking whether gperf is recent enough... "])
  if "$GPERF" < src/conf-tokens.gperf >/dev/null 2>&1 ; then
    AS_ECHO("yes")
    AC_DEFINE(HAVE_GPERF)
  else
    AM_CONDITIONAL(HAVE_GPERF, false)
    AS_ECHO("no")
  fi
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
data/templates/Makefile
etc/Makefile
docs/Makefile
docs/man5/Makefile
docs/man5/tinyproxy.conf.txt
docs/man8/Makefile
docs/man8/tinyproxy.txt
m4macros/Makefile
tests/Makefile
tests/scripts/Makefile
scripts/Makefile
])

AC_OUTPUT

# the manpages are shipped in the release tarball and we don't want them to
# get regenerated if pod2man is not available. the intermediate files from
# AC_CONFIG_FILES are created with config.status, which is created at configure
# runtime, so we need to touch them after config.status terminated to prevent
# make from rebuild them.

if test "x$POD2MAN" = "xno" ; then
  touch docs/man5/tinyproxy.conf.txt
  touch docs/man8/tinyproxy.txt
  if test -e docs/man5/tinyproxy.conf.5 ; then
      touch docs/man5/tinyproxy.conf.5
  fi
  if test -e docs/man8/tinyproxy.8 ; then
      touch docs/man8/tinyproxy.8
  fi
fi
