dnl $Id: configure.ac,v 2.7 2002-04-07 21:31:15 rjkaes Exp $

dnl Devlopers, please strive to achieve this order:
dnl
dnl 0. Initialization and options processing
dnl 1. Programs
dnl 2. Libraries
dnl 3. Header files
dnl 4. Types
dnl 5. Structures
dnl 6. Compiler characteristics
dnl 7. Functions, global variables
dnl 8. System services
dnl
dnl Read the Autoconf manual for details.

AC_INIT([tinyproxy], [1.4.4pre1], [rjkaes@users.sourceforge.net])
AC_CONFIG_SRCDIR([src/tinyproxy.c])
AC_PREREQ(2.52)

AC_CANONICAL_TARGET

AH_TEMPLATE([TARGET_SYSTEM],
	    [A string containing the target system for which tinyproxy was built.])
AC_DEFINE_UNQUOTED(TARGET_SYSTEM, ["$target"])

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

dnl Check if we're compiling on a weird platform :)
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl
dnl Command line options
dnl

dnl Set the URL name for find the statistics of tinyproxy
AH_VERBATIM([DEFAULT_STATHOST],
[/*
 * This controls remote proxy stats deplay. For example, the default 
 * DEFAULT_STATHOST of "tinyproxy.stats" means that where you access
 * "http://tinyproxy.stats/" you will be shown the proxy stats. Use "" to
 * disable.
 */
#ifndef DEFAULT_STATHOST
# define DEFAULT_STATHOST "tinyproxy.stats"
#endif])
AC_ARG_WITH(stathost,
            [AC_HELP_STRING([--with-stathost=HOST], [Default status host])],
            [AC_DEFINE_UNQUOTED(DEFAULT_STATHOST, ["$withval"])])

dnl Set the default configuration file location
AH_VERBATIM([DEFAULT_CONF_FILE],
[/* This is the default location of the configuation file. */
#ifndef DEFAULT_CONF_FILE
# define DEFAULT_CONF_FILE "/etc/tinyproxy/tinyproxy.conf"
#endif])
AC_ARG_WITH(config,
            [AC_HELP_STRING([--with-config=FILE],
                            [Set the default location of the config file])],
	    [AC_DEFINE_UNQUOTED(DEFAULT_CONF_FILE, ["$withval"])])

dnl Add compiler-specific optimization flags
AC_ARG_ENABLE(debug,
              [AC_HELP_STRING([--enable-debug],
                              [Disable aggressive optimizations (default is NO)])],
              debug_enabled=$enableval, debug_enabled=no)

dnl Add in profiling information
AC_ARG_ENABLE(profiling,
              [AC_HELP_STRING([--enable-profiling],
                              [Enable profiling information (default is NO)])],
              profiling_enabled=$enableval, profiling_enabled=no)

dnl Include SOCKS support
AC_ARG_ENABLE(socks,
              [AC_HELP_STRING([--enable-socks],
                              [Enable SOCKS support (default is NO)])],
              socks_enabled=$enableval, socks_enabled=no)

dnl Check to see if the XTinyproxy header is to be included
AC_ARG_ENABLE(xtinyproxy,
              [AC_HELP_STRING([--enable-xtinyproxy],
                              [Include X-Tinyproxy header (default is YES)])],
              xtinyproxy_enabled=$enableval, xtinyproxy_enabled=yes)

if test x"$xtinyproxy_enabled" = x"yes"; then
    AH_TEMPLATE([XTINYPROXY_ENABLE],
                [Define if you want to have the peer's IP address included in
		 a XTinyproxy header sent to the server.])
    AC_DEFINE(XTINYPROXY_ENABLE)
fi

dnl Include filtering for domain/URLs
AC_ARG_ENABLE(filter,
              [AC_HELP_STRING([--enable-filter],
                              [Enable filtering of domains/URLs (default is YES)])],
              filter_enabled=$enableval, filter_enabled=yes)

if test x"$filter_enabled" = x"yes"; then
    LIBOBJS="filter.o $LIBOBJS"

    AH_TEMPLATE([FILTER_ENABLE],
		[Defined if you would like filtering code included.])

    AC_DEFINE(FILTER_ENABLE)
fi

dnl Include support for TCP tunneling
AC_ARG_ENABLE(tunnel,
              [AC_HELP_STRING([--enable-tunnel],
                              [Enable support for TCP tunneling (default is YES)])],
              tunnel_enabled=$enableval, tunnel_enabled=yes)
if test x"$tunnel_enabled" = x"yes" ; then
    AH_TEMPLATE([TUNNEL_SUPPORT],
		[Include TCP tunnelling support?])

    AC_DEFINE(TUNNEL_SUPPORT)
fi

dnl Include support for upstream proxies?
AC_ARG_ENABLE(upstream,
              [AC_HELP_STRING([--enable-upstream],
                              [Enable support for upstream proxies (default is YES)])],
              upstream_enabled=$enableval, upstream_enabled=yes)
if test x"$upstream_enabled" = x"yes"; then
    AH_TEMPLATE([UPSTREAM_SUPPORT],
		[Include support for connecting to an upstream proxy.])

    AC_DEFINE(UPSTREAM_SUPPORT)
fi

dnl Include support for a statically compiled tinyproxy
AC_ARG_ENABLE(static,
              [AC_HELP_STRING([--enable-static],
                              [Statically link tinyproxy (default is NO)])],
	      [LDFLAGS="-static $LDFLAGS"])

dnl
dnl Checks for programs
dnl

dnl Save $CFLAGS since AC_PROG_CC likes to insert "-g -O2"
dnl if $CFLAGS is blank
cflags_save="$CFLAGS"

AC_PROG_AWK
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_LEX
AC_PROG_YACC

CFLAGS="$cflags_save"

dnl Make sure YACC is actually bison
if test x"$YACC" != x"bison -y"; then
    AC_MSG_WARN([You will need bison if you want to regenerate the parser.])
fi

dnl
dnl Checks for libraries
dnl
AC_CHECK_LIB(pthread, pthread_create)
if test x"$ac_cv_lib_pthread_pthread_create" = x"yes"; then
    CFLAGS="-D_REENTRANT $CFLAGS"
else
    AC_CHECK_LIB(pthreads, pthread_create)
    if test x"$ac_cv_lib_pthreads_pthread_create" = x"yes"; then
        CFLAGS="-D_REENTRANT $CFLAGS"
    else
        AC_CHECK_LIB(c_r, pthread_create,
		  USE_PTHREAD=1 PTHREAD_LDFLAGS="-lc_r")
        if test x"$ac_cv_lib_c_r_pthread_create" = x"yes"; then
            CFLAGS="-pthread -D_REENTRANT $CFLAGS"
        else
	    AC_MSG_ERROR([You must have a POSIX compliant threading library installed])
        fi
		case "$target" in
		*-freebsd*)
			;;
		*)
			LIBS = "$LIBS -lc_r"
			;;
		esac
    fi
fi

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
AC_CHECK_LIB(socks, main, socks_library=yes, socks_library=no)

dnl
dnl Checks for headers
dnl
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/types.h sys/ioctl.h sys/resource.h sys/select.h \
		  sys/socket.h sys/stat.h sys/time.h sys/uio.h \
		  sys/un.h sys/wait.h arpa/inet.h netinet/in.h alloca.h \
		  assert.h ctype.h errno.h fcntl.h grp.h io.h libintl.h \
		  malloc.h memory.h netdb.h pthread.h pwd.h regex.h signal.h \
		  stdarg.h stddef.h stdint.h stdio.h stdlib.h string.h \
		  strings.h sysexits.h syslog.h time.h unistd.h wchar.h \
		  wctype.h])

AC_CHECK_HEADER(socks.h, socks_header=yes, socks_header=no)

dnl
dnl Checks for types
dnl
AM_C_PROTOTYPES
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_UNP_CHECK_TYPE(uint8_t, unsigned char)
AC_UNP_CHECK_TYPE(int16_t, short)
AC_UNP_CHECK_TYPE(uint16_t, unsigned short)
AC_UNP_CHECK_TYPE(int32_t, int)
AC_UNP_CHECK_TYPE(uint32_t, unsigned int)
AC_UNP_CHECK_TYPE(ssize_t, int)
AC_UNP_CHECK_TYPE(socklen_t, unsigned int)
AC_UNP_CHECK_TYPE(in_addr_t, uint32_t)
AC_TYPE_PID_T

dnl
dnl Checks for libary functions
dnl
AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_MALLOC
dnl AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_STRERROR_R
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([btowc bzero ftruncate gethostbyaddr gethostbyname gethostname \
		inet_ntoa memchr memset re_comp regcomp regexec select \
		setrlimit socket strncasecmp strchr strdup strerror strstr \
		strtol vsyslog vsnprintf])

AC_CHECK_FUNCS(strlcpy strlcat)

dnl
dnl Compiler characteristics
dnl

dnl If profiling is enabled, then enable the debugging code
if test x"$profiling_enabled" = x"yes" ; then
    AC_MSG_NOTICE([profiling is enabled, therefore enabling debugging code.])
    debug_enabled=yes
fi

dnl Enable the debugging flags (by checking for the GCC compiler)
CFLAGS="-O2 $CFLAGS"
if test x"$debug_enabled" = x"yes" ; then
    dnl Add the warnings if we have the GCC compiler
    if test x"$GCC" = x"yes" ; then
        if test x"$profiling_enabled" = x"yes" ; then
	    CFLAGS="-pg -fprofile-arcs $CFLAGS"
        fi
        CFLAGS="-Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes $CFLAGS"
        CFLAGS="-Wmissing-prototypes -Wmissing-declarations $CFLAGS"
        CFLAGS="-Wpointer-arith -Waggregate-return -Wnested-externs $CFLAGS"
    fi
    CFLAGS="-Wall -g -DYYDEBUG $CFLAGS"
    YFLAGS="-v -d"
else
    dnl No debugging information, include the optimizations
    CFLAGS="-DNDEBUG $CFLAGS"
    YFLAGS="-d"
fi

dnl
dnl Make sure we can actually handle the "--with-*" and "--enable-*" stuff.
dnl

dnl Handle the SOCKS support
if test x"$socks_enabled" = x"yes"; then
    if test x"$socks_header" = x"yes" -a x"$socks_library" = x"yes"; then
        CFLAGS="-I/usr/include/sock.h -DSOCKS $CFLAGS"
        LIBS="-lsocks $LIBS"
    else
        AC_MSG_ERROR([Could not include the SOCKS library or header])
    fi
fi

dnl Handle the REGEX library
if test x"$ac_cv_func_regexec" != x"yes"; then
    dnl We don't have a functioning REGEX so include our copy
    tinyproxy_use_our_regex=yes
else
    AC_MSG_CHECKING([whether the system's regex library is broken])
    AC_CACHE_VAL(tinyproxy_cv_regex_broken,
                 [AC_TRY_RUN([
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_REGEX_H
# include <regex.h>
#endif
int main(void)
{
	regex_t blah;
	return regcomp(&blah, "foo.*bar", REG_NOSUB) || regexec(&blah, "foobar", 0, NULL, 0);
}
                            ],
                            tinyproxy_cv_regex_broken=no,
                            tinyproxy_cv_regex_broken=yes,
                            tinyproxy_cv_regex_broken=yes)])

    AC_MSG_RESULT([$tinyproxy_cv_regex_broken])

    if test x"$tinyproxy_cv_regex_broken" = x"yes" ; then
        AC_MSG_NOTICE([Using the included GNU regex instead.])
        tinyproxy_use_our_regex=yes
    fi
fi

if test x"$tinyproxy_use_our_regex" = x"yes" ; then
    AH_TEMPLATE([USE_GNU_REGEX],
		[Defined if you want to use the included regex routines.])

    AC_DEFINE(USE_GNU_REGEX)
    LIBOBJS="gnuregex.o $LIBOBJS"
fi

dnl
dnl Substitute the variables into the various Makefiles
dnl
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(YFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LIBOBJS)

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])
AC_OUTPUT
