AC_PREREQ(2.54)

AC_INIT([Tinyproxy], [1.7.1], [https://www.banu.com/bugzilla/enter_bug.cgi?product=Tinyproxy], [tinyproxy])
dnl AC_CONFIG_AUX_DIR(config)
dnl AC_CONFIG_MACRO_DIR(config)

AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4macros])
AM_MAINTAINER_MODE

AH_TEMPLATE([TARGET_SYSTEM],
	    [A string containing the target system for which tinyproxy was built.])
AC_DEFINE_UNQUOTED(TARGET_SYSTEM, ["$target"])

dnl Check if we're compiling on a weird platform :)
AC_USE_SYSTEM_EXTENSIONS

dnl
dnl Command line options
dnl

dnl Set the URL name for find the statistics of tinyproxy
AH_TEMPLATE([DEFAULT_STATHOST],
	[This controls remote proxy stats display.])
AC_ARG_WITH(stathost,
            [AC_HELP_STRING([--with-stathost=HOST], [Default status host])],
            [AC_DEFINE_UNQUOTED(DEFAULT_STATHOST, "$withval")],
	    [AC_DEFINE_UNQUOTED(DEFAULT_STATHOST, "tinyproxy.stats")])

dnl Set the default configuration file location
test "$prefix" = "NONE" && prefix=/usr/local

AH_TEMPLATE([DEFAULT_CONF_FILE],
	[This is the default file name for the configuration file.])
AC_ARG_WITH(config,
            [AC_HELP_STRING([--with-config=FILE],
                            [Set the default location of the config file])],
	    [tp_config_file="$withval"],
	    [tp_config_file="tinyproxy.conf"])

# If the first character is a slash, it's an absolute path
if [[ `echo $tp_config_file | cut -c1` = "/" ]]; then
	TINYPROXY_CONFIG_DIR=`echo "$tp_config_file" | sed 's/^\(.*\)\/[[^\/]]*$/\1/'`
        TINYPROXY_CONFIG_FILE=`echo "$tp_config_file" | sed 's/^.*\/\([[^\/]]*\)$/\1/'`
else
	# If sysconfdir is /usr/etc, change it to /etc (since /usr/etc doesn't exist)
	if [[ "/usr/etc" = `eval echo $sysconfdir` -a "/usr" = "$prefix" ]]; then
		TINYPROXY_CONFIG_DIR="/etc/tinyproxy"
	else
		TINYPROXY_CONFIG_DIR=`eval echo $sysconfdir/tinyproxy`
	fi
        TINYPROXY_CONFIG_FILE="$tp_config_file"
fi

AC_DEFINE_UNQUOTED(DEFAULT_CONF_FILE, "$TINYPROXY_CONFIG_DIR/$TINYPROXY_CONFIG_FILE")

AC_MSG_NOTICE([Default config location and file is: $TINYPROXY_CONFIG_DIR/$TINYPROXY_CONFIG_FILE])

dnl Add compiler-specific optimization flags
TP_ARG_ENABLE(debug,
              [Enable debugging support code and methods (default is NO)],
              no)

dnl Add in profiling information
TP_ARG_ENABLE(profiling,
              [Enable profiling information (default is NO)],
              no)

dnl Include SOCKS support
TP_ARG_ENABLE(socks,
              [Enable SOCKS support (default is NO)],
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
              [Enable reverse proxying (default is NO)],
              no)
if test x"$reverse_enabled" = x"yes"; then
    ADDITIONAL_OBJECTS="$ADDITIONAL_OBJECTS reverse-proxy.o"
    AC_DEFINE(REVERSE_SUPPORT)
fi

dnl Include the transparent proxy support
AH_TEMPLATE([TRANSPARENT_PROXY],
	    [Include support for using tinyproxy as a transparent proxy.])
TP_ARG_ENABLE(transparent,
              [Enable transparent proxying code (default is NO)],
              no)
if test x"$transparent_enabled" = x"yes"; then
   ADDITIONAL_OBJECTS="$ADDITIONAL_OBJECTS transparent-proxy.o"
   AC_DEFINE(TRANSPARENT_PROXY)
fi

dnl
dnl Checks for programs
dnl

AC_PROG_MAKE_SET
AC_PROG_CC
AC_ISC_POSIX
AC_PROG_INSTALL

dnl If a CFLAGS variable was passed during configure, set the initial
dnl CFLAGS variable to it; otherwise, start with an empty CFLAGS
if test x"$ac_env_CFLAGS_set" = x"set" ; then
    CFLAGS=$ac_env_CFLAGS_value
else
    CFLAGS=""
fi

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
AC_CHECK_LIB(socks, main, socks_library=yes, socks_library=no)

dnl
dnl Checks for headers
dnl
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([sys/ioctl.h sys/mman.h sys/resource.h \
		  sys/select.h sys/socket.h sys/time.h sys/uio.h \
		  sys/un.h arpa/inet.h netinet/in.h \
		  assert.h ctype.h errno.h fcntl.h grp.h io.h libintl.h \
		  netdb.h pwd.h regex.h signal.h stdarg.h stddef.h stdio.h \
		  sysexits.h syslog.h time.h wchar.h wctype.h \
		  values.h])

dnl OpenBSD machines don't like having malloc included (even if it's present)
dnl as they expect you to use stdlib.h
case "$target" in
    *-openbsd*) ;;
    *)         AC_CHECK_HEADER(malloc.h);;
esac

AC_CHECK_HEADER(socks.h, socks_header=yes, socks_header=no)


dnl
dnl Checks for types
dnl
AM_C_PROTOTYPES
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_UNP_CHECK_TYPE(uint8_t, unsigned char)
AC_UNP_CHECK_TYPE(int16_t, short)
AC_UNP_CHECK_TYPE(uint16_t, unsigned short)
AC_UNP_CHECK_TYPE(int32_t, int)
AC_UNP_CHECK_TYPE(uint32_t, unsigned int)
AC_UNP_CHECK_TYPE(ssize_t, int)
AC_UNP_CHECK_TYPE(socklen_t, unsigned int)
AC_UNP_CHECK_TYPE(in_addr_t, uint32_t)


dnl
dnl Checks for libary functions
dnl
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME

AC_CHECK_FUNCS([gethostname inet_ntoa memchr memset select socket strcasecmp \
                strchr strdup strerror strncasecmp strpbrk strstr strtol])
AC_CHECK_FUNCS([isascii memcpy setrlimit ftruncate regcomp regexec])
AC_CHECK_FUNCS([strlcpy strlcat])

dnl
dnl Compiler characteristics
dnl

dnl If profiling is enabled, then enable the debugging code
if test x"$profiling_enabled" = x"yes" ; then
    AC_MSG_NOTICE([profiling is enabled, therefore enabling debugging code.])
    debug_enabled=yes
fi

dnl Enable the debugging flags (by checking for the GCC compiler)
if test x"$debug_enabled" = x"yes" ; then
    dnl Add the warnings if we have the GCC compiler
    if test x"$GCC" = x"yes" ; then
        if test x"$profiling_enabled" = x"yes" ; then
	    CFLAGS="-pg -fprofile-arcs $CFLAGS"
        fi
        CFLAGS="-Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes $CFLAGS"
        CFLAGS="-Wmissing-prototypes -Wmissing-declarations $CFLAGS"
        CFLAGS="-Wpointer-arith -Waggregate-return -Wnested-externs $CFLAGS"
        CFLAGS="-Wwrite-strings -Wcomment -Wextra -Wc++-compat $CFLAGS"
    fi
    CFLAGS="-Wall -g $CFLAGS"
else
    dnl No debugging information, include the optimizations
    CFLAGS="-O2 -DNDEBUG $CFLAGS"
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
    AC_MSG_ERROR([Could not locate the regexec() function])
else
    AC_MSG_CHECKING([whether the system's regex library is broken])
    AC_CACHE_VAL(tinyproxy_cv_regex_broken,
                 [AC_TRY_RUN([
#if HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_REGEX_H
# include <regex.h>
#endif
int main(void)
{
	regex_t blah;
	if (regcomp(&blah, "foo.*bar", REG_NOSUB) != 0)
		exit(1);
	if (regexec(&blah, "foobar", 0, NULL, 0) != 0)
		exit(1);
	else
		exit(0);

	return 0;
}
                            ],
                            tinyproxy_cv_regex_broken=no,
                            tinyproxy_cv_regex_broken=yes,
                            tinyproxy_cv_regex_broken=yes)])

    AC_MSG_RESULT([$tinyproxy_cv_regex_broken])

    if test x"$tinyproxy_cv_regex_broken" = x"yes" ; then
        AC_MSG_ERROR([Your system's regexec() function is broken.])
    fi
fi

dnl
dnl Substitute the variables into the various Makefiles
dnl
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LIBS)
AC_SUBST(ADDITIONAL_OBJECTS)
AC_SUBST(TINYPROXY_CONFIG_DIR)
AC_SUBST(TINYPROXY_CONFIG_FILE)

# Check for asciidoc
AC_PATH_PROG(A2X, a2x, no)
AM_CONDITIONAL(HAVE_A2X, test "x$A2X" != "xno")

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
data/templates/Makefile
etc/Makefile
doc/Makefile
doc/man8/Makefile
m4macros/Makefile
packaging/Makefile
packaging/fedora/tinyproxy.spec
packaging/fedora/Makefile
])

AC_OUTPUT
