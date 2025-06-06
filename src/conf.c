/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2004 Robert James Kaes <rjkaes@users.sourceforge.net>
 * Copyright (C) 2009 Michael Adam <obnox@samba.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Parses the configuration file and sets up the config_s structure for
 * use by the application.  This file replaces the old grammar.y and
 * scanner.l files.  It takes up less space and _I_ think is easier to
 * add new directives to.  Who knows if I'm right though.
 */

#include "common.h"
#include <regex.h>
#include "conf.h"

#include "acl.h"
#include "anonymous.h"
#include "filter.h"
#include "heap.h"
#include "html-error.h"
#include "log.h"
#include "reqs.h"
#include "reverse-proxy.h"
#include "upstream.h"
#include "connect-ports.h"
#include "basicauth.h"
#include "conf-tokens.h"

#ifdef LINE_MAX
#define TP_LINE_MAX LINE_MAX
#else
#define TP_LINE_MAX 1024
#endif

/*
 * The configuration directives are defined in the structure below.  Each
 * directive requires a regular expression to match against, and a
 * function to call when the regex is matched.
 *
 * Below are defined certain constant regular expression strings that
 * can (and likely should) be used when building the regex for the
 * given directive.
 */
#define DIGIT "[0-9]"
#define SPACE "[ \t]"
#define WS SPACE "+"
#define STR "\"([^\"]+)\""
#define BOOL "(yes|on|no|off)"
#define INT "(()" DIGIT "+)"
#define ALNUM "([-a-z0-9._]+)"
#define USERNAME "([^:]*)"
#define PASSWORD "(.*)"
#define IP "((([0-9]{1,3})\\.){3}[0-9]{1,3})"
#define IPMASK "(" IP "(/" DIGIT "+)?)"
#define IPV6SCOPE "((%[^ \t\\/]{1,16})?)"
#define IPV6 "(" \
        "([0-9a-f:]{2,39})" IPV6SCOPE "|" \
        "([0-9a-f:]{0,29}:" IP ")" IPV6SCOPE \
        ")"

#define IPV6MASK "(" IPV6 "(/" DIGIT "+)?)"
#define BEGIN "^" SPACE "*"
#define END SPACE "*$"

/*
 * Limit the maximum number of substring matches to a reasonably high
 * number.  Given the usual structure of the configuration file, sixteen
 * substring matches should be plenty.
 */
#define RE_MAX_MATCHES 33

#define CP_WARN(FMT, ...) \
        log_message (LOG_WARNING, "line %lu: " FMT, lineno, __VA_ARGS__)

/*
 * All configuration handling functions are REQUIRED to be defined
 * with the same function template as below.
 */
typedef int (*CONFFILE_HANDLER) (struct config_s *, const char *,
             unsigned long, regmatch_t[]);

/*
 * Define the pattern used by any directive handling function.  The
 * following arguments are defined:
 *
 *   struct config_s* conf   pointer to the current configuration structure
 *   const char* line          full line matched by the regular expression
 *   regmatch_t match[]        offsets to the substrings matched
 *
 * The handling function must return 0 if the directive was processed
 * properly.  Any errors are reported by returning a non-zero value.
 */
#define HANDLE_FUNC(func) \
  int func(struct config_s* conf, const char* line, \
           unsigned long lineno, regmatch_t match[])

/*
 * List all the handling functions.  These are defined later, but they need
 * to be in-scope before the big structure below.
 */
static HANDLE_FUNC (handle_disabled_feature)
{
        fprintf (stderr, "ERROR: accessing feature that was disabled at compiletime on line %lu\n",
                 lineno);

        return -1;
}

static HANDLE_FUNC (handle_allow);
static HANDLE_FUNC (handle_basicauth);
static HANDLE_FUNC (handle_basicauthrealm);
static HANDLE_FUNC (handle_anonymous);
static HANDLE_FUNC (handle_bind);
static HANDLE_FUNC (handle_bindsame);
static HANDLE_FUNC (handle_connectport);
static HANDLE_FUNC (handle_defaulterrorfile);
static HANDLE_FUNC (handle_deny);
static HANDLE_FUNC (handle_errorfile);
static HANDLE_FUNC (handle_addheader);
#ifdef FILTER_ENABLE
static HANDLE_FUNC (handle_filter);
static HANDLE_FUNC (handle_filtercasesensitive);
static HANDLE_FUNC (handle_filterdefaultdeny);
static HANDLE_FUNC (handle_filterextended);
static HANDLE_FUNC (handle_filterurls);
static HANDLE_FUNC (handle_filtertype);
#endif
static HANDLE_FUNC (handle_group);
static HANDLE_FUNC (handle_listen);
static HANDLE_FUNC (handle_logfile);
static HANDLE_FUNC (handle_loglevel);
static HANDLE_FUNC (handle_maxclients);
static HANDLE_FUNC (handle_obsolete);
static HANDLE_FUNC (handle_pidfile);
static HANDLE_FUNC (handle_port);
#ifdef REVERSE_SUPPORT
static HANDLE_FUNC (handle_reversebaseurl);
static HANDLE_FUNC (handle_reversemagic);
static HANDLE_FUNC (handle_reverseonly);
static HANDLE_FUNC (handle_reversepath);
#endif
static HANDLE_FUNC (handle_statfile);
static HANDLE_FUNC (handle_stathost);
static HANDLE_FUNC (handle_syslog);
static HANDLE_FUNC (handle_timeout);

static HANDLE_FUNC (handle_user);
static HANDLE_FUNC (handle_viaproxyname);
static HANDLE_FUNC (handle_disableviaheader);
static HANDLE_FUNC (handle_xtinyproxy);

#ifdef UPSTREAM_SUPPORT
static HANDLE_FUNC (handle_upstream);
#endif

static void config_free_regex (void);

/*
 * This macro can be used to make standard directives in the form:
 *   directive arguments [arguments ...]
 *
 * The directive itself will be the first matched substring.
 *
 * Note that this macro is not required.  As you can see below, the
 * comment and blank line elements are defined explicitly since they
 * do not follow the pattern above.  This macro is for convenience
 * only.
 */
#define STDCONF(d, re, func) [CD_ ## d] = { BEGIN "()" WS re END, func, NULL }

/*
 * Holds the regular expression used to match the configuration directive,
 * the function pointer to the routine to handle the directive, and
 * for internal use, a pointer to the compiled regex so it only needs
 * to be compiled one.
 */
struct {
        const char *re;
        CONFFILE_HANDLER handler;
        regex_t *cre;
} directives[] = {
        /* string arguments */
        STDCONF (basicauthrealm, STR, handle_basicauthrealm),
        STDCONF (logfile, STR, handle_logfile),
        STDCONF (pidfile, STR, handle_pidfile),
        STDCONF (anonymous, STR, handle_anonymous),
        STDCONF (viaproxyname, STR, handle_viaproxyname),
        STDCONF (defaulterrorfile, STR, handle_defaulterrorfile),
        STDCONF (statfile, STR, handle_statfile),
        STDCONF (stathost, STR, handle_stathost),
        STDCONF (xtinyproxy,  BOOL, handle_xtinyproxy),
        /* boolean arguments */
        STDCONF (syslog, BOOL, handle_syslog),
        STDCONF (bindsame, BOOL, handle_bindsame),
        STDCONF (disableviaheader, BOOL, handle_disableviaheader),
        /* integer arguments */
        STDCONF (port, INT, handle_port),
        STDCONF (maxclients, INT, handle_maxclients),
        STDCONF (maxspareservers, INT, handle_obsolete),
        STDCONF (minspareservers, INT, handle_obsolete),
        STDCONF (startservers, INT, handle_obsolete),
        STDCONF (maxrequestsperchild, INT, handle_obsolete),
        STDCONF (timeout, INT, handle_timeout),
        STDCONF (connectport, INT, handle_connectport),
        /* alphanumeric arguments */
        STDCONF (user, ALNUM, handle_user),
        STDCONF (group, ALNUM, handle_group),
        /* ip arguments */
        STDCONF (listen, "(" IP "|" IPV6 ")", handle_listen),
        STDCONF (allow, "(" "(" IPMASK "|" IPV6MASK ")" "|" ALNUM ")",
                 handle_allow),
        STDCONF (deny, "(" "(" IPMASK "|" IPV6MASK ")" "|" ALNUM ")",
                 handle_deny),
        STDCONF (bind, "(" IP "|" IPV6 ")", handle_bind),
        /* other */
        STDCONF (basicauth, USERNAME WS PASSWORD, handle_basicauth),
        STDCONF (errorfile, INT WS STR, handle_errorfile),
        STDCONF (addheader,  STR WS STR, handle_addheader),

#ifdef FILTER_ENABLE
        /* filtering */
        STDCONF (filter, STR, handle_filter),
        STDCONF (filterurls, BOOL, handle_filterurls),
        STDCONF (filterextended, BOOL, handle_filterextended),
        STDCONF (filterdefaultdeny, BOOL, handle_filterdefaultdeny),
        STDCONF (filtercasesensitive, BOOL, handle_filtercasesensitive),
        STDCONF (filtertype, "(bre|ere|fnmatch)", handle_filtertype),
#endif
#ifdef REVERSE_SUPPORT
        /* Reverse proxy arguments */
        STDCONF (reversebaseurl, STR, handle_reversebaseurl),
        STDCONF (reverseonly, BOOL, handle_reverseonly),
        STDCONF (reversemagic, BOOL, handle_reversemagic),
        STDCONF (reversepath, STR "(" WS STR ")?", handle_reversepath),
#endif
#ifdef UPSTREAM_SUPPORT
        STDCONF (upstream,
                 "(" "(none)" WS STR ")|" \
                 "(" "(http|socks4|socks5)" WS \
                     "(" USERNAME /*username*/ ":" PASSWORD /*password*/ "@" ")?"
                     "(" IP "|" "\\[(" IPV6 ")\\]" "|" ALNUM ")"
                     ":" INT "(" WS STR ")?" ")", handle_upstream),
#endif
        /* loglevel */
        STDCONF (loglevel, "(critical|error|warning|notice|connect|info)",
                 handle_loglevel)
};

const unsigned int ndirectives = sizeof (directives) / sizeof (directives[0]);

static void
free_added_headers (sblist* add_headers)
{
        size_t i;

        if (!add_headers) return;

        for (i = 0; i < sblist_getsize (add_headers); i++) {
                http_header_t *header = sblist_get (add_headers, i);

                safefree (header->name);
                safefree (header->value);
        }

        sblist_free (add_headers);
}

static void stringlist_free(sblist *sl) {
        size_t i;
        char **s;
        if(sl) {
                for(i = 0; i < sblist_getsize(sl); i++) {
                        s = sblist_get(sl, i);
                        if(s) safefree(*s);
		}
                sblist_free(sl);
        }
}

void free_config (struct config_s *conf)
{
        char *k;
        htab_value *v;
        size_t it;
        safefree (conf->basicauth_realm);
        safefree (conf->logf_name);
        safefree (conf->stathost);
        safefree (conf->user);
        safefree (conf->group);
        stringlist_free(conf->basicauth_list);
        stringlist_free(conf->listen_addrs);
        stringlist_free(conf->bind_addrs);
#ifdef FILTER_ENABLE
        safefree (conf->filter);
#endif                          /* FILTER_ENABLE */
#ifdef REVERSE_SUPPORT
        free_reversepath_list(conf->reversepath_list);
        safefree (conf->reversebaseurl);
#endif
#ifdef UPSTREAM_SUPPORT
        free_upstream_list (conf->upstream_list);
#endif                          /* UPSTREAM_SUPPORT */
        safefree (conf->pidpath);
        safefree (conf->via_proxy_name);
        if (conf->errorpages) {
                it = 0;
                while((it = htab_next(conf->errorpages, it, &k, &v))) {
                        safefree(k);
                        safefree(v->p);
                }
                htab_destroy (conf->errorpages);
        }
        free_added_headers (conf->add_headers);
        safefree (conf->errorpage_undef);
        safefree (conf->statpage);
        flush_access_list (conf->access_list);
        free_connect_ports_list (conf->connect_ports);
        if (conf->anonymous_map) {
                it = 0;
                while((it = htab_next(conf->anonymous_map, it, &k, &v)))
                       safefree(k);
                htab_destroy (conf->anonymous_map);
        }

        memset (conf, 0, sizeof(*conf));
}

/*
 * Initializes Config parser. Currently this means:
 * Compiles the regular expressions used by the configuration file.  This
 * routine MUST be called before trying to parse the configuration file.
 *
 * Returns 0 on success; negative upon failure.
 */
int
config_init (void)
{
        unsigned int i, r;

        for (i = 0; i != ndirectives; ++i) {
                assert (!directives[i].cre);

                if (!directives[i].handler) {
                        directives[i].handler = handle_disabled_feature;
                        continue;
                }

                directives[i].cre = (regex_t *) safemalloc (sizeof (regex_t));
                if (!directives[i].cre)
                        return -1;

                r = regcomp (directives[i].cre,
                             directives[i].re,
                             REG_EXTENDED | REG_ICASE | REG_NEWLINE);
                if (r)
                        return r;
        }

        atexit (config_free_regex);

        return 0;
}

/*
 * Frees pre-compiled regular expressions used by the configuration
 * file. This function is registered to be automatically called at exit.
 */
static void
config_free_regex (void)
{
        unsigned int i;

        for (i = 0; i < ndirectives; i++) {
                if (directives[i].cre) {
                        regfree (directives[i].cre);
                        safefree (directives[i].cre);
                        directives[i].cre = NULL;
                }
        }
}

/*
 * Attempt to match the supplied line with any of the configuration
 * regexes defined above.  If a match is found, call the handler
 * function to process the directive.
 *
 * Returns 0 if a match was found and successfully processed; otherwise,
 * a negative number is returned.
 */
static int check_match (struct config_s *conf, const char *line,
                        unsigned long lineno, enum config_directive cd)
{
        regmatch_t match[RE_MAX_MATCHES];
        unsigned int i = cd;

        if (!directives[i].cre)
                return (*directives[i].handler) (conf, line, lineno, match);

        if (!regexec
            (directives[i].cre, line, RE_MAX_MATCHES, match, 0))
                return (*directives[i].handler) (conf, line, lineno, match);
        return -1;
}

/*
 * Parse the previously opened configuration stream.
 */
static int config_parse (struct config_s *conf, FILE * f)
{
        char buffer[TP_LINE_MAX], *p, *q, c;
        const struct config_directive_entry *e;
        unsigned long lineno = 1;

        for (;fgets (buffer, sizeof (buffer), f);++lineno) {
                if(buffer[0] == '#') continue;
                p = buffer;
                while(isspace(*p))p++;
                if(!*p) continue;
                q = p;
                while(*q && !isspace(*q))q++;
                c = *q;
                *q = 0;
                e = config_directive_find(p, strlen(p));
                *q = c;
                if (!e || e->value == CD_NIL || check_match (conf, q, lineno, e->value)) {
                        fprintf (stderr, "ERROR: Syntax error on line %lu\n", lineno);
                        return 1;
                }
        }
        return 0;
}

/**
 * Read the settings from a config file.
 */
static int load_config_file (const char *config_fname, struct config_s *conf)
{
        FILE *config_file;
        int ret = -1;

        config_file = fopen (config_fname, "r");
        if (!config_file) {
                fprintf (stderr,
                         "%s: Could not open config file \"%s\".\n",
                         PACKAGE, config_fname);
                goto done;
        }

        if (config_parse (conf, config_file)) {
                fprintf (stderr, "Unable to parse config file. "
                         "Not starting.\n");
                goto done;
        }

        ret = 0;

done:
        if (config_file)
                fclose (config_file);

        return ret;
}

static void initialize_config_defaults (struct config_s *conf)
{
        memset (conf, 0, sizeof(*conf));

        /*
         * Make sure the HTML error pages array is NULL to begin with.
         * (FIXME: Should have a better API for all this)
         */
        conf->errorpages = NULL;
        conf->basicauth_realm = safestrdup (PACKAGE_NAME);
        conf->stathost = safestrdup (TINYPROXY_STATHOST);
        conf->idletimeout = MAX_IDLE_TIME;
        conf->logf_name = NULL;
        conf->pidpath = NULL;
        conf->maxclients = 100;
}

/**
 * Load the configuration.
 */
int reload_config_file (const char *config_fname, struct config_s *conf)
{
        int ret;

        initialize_config_defaults (conf);

        ret = load_config_file (config_fname, conf);
        if (ret != 0) {
                goto done;
        }

        /* Set the default values if they were not set in the config file. */
        if (conf->port == 0) {
                /*
                 * Don't log here in error path:
                 * logging might not be set up yet!
                 */
                fprintf (stderr, PACKAGE ": You MUST set a Port in the "
                         "config file.\n");
                ret = -1;
                goto done;
        }

        if (!conf->user && !geteuid()) {
                log_message (LOG_WARNING, "You SHOULD set a UserName in the "
                             "config file. Using current user instead.");
        }

        if (conf->idletimeout == 0) {
                log_message (LOG_WARNING, "Invalid idle time setting. "
                             "Only values greater than zero are allowed. "
                             "Therefore setting idle timeout to %u seconds.",
                             MAX_IDLE_TIME);
                conf->idletimeout = MAX_IDLE_TIME;
        }

done:
        return ret;
}

/***********************************************************************
 *
 * The following are basic data extraction building blocks that can
 * be used to simplify the parsing of a directive.
 *
 ***********************************************************************/

static char *get_string_arg (const char *line, regmatch_t * match)
{
        char *p;
        const unsigned int len = match->rm_eo - match->rm_so;

        assert (line);
        assert (len > 0);

        p = (char *) safemalloc (len + 1);
        if (!p)
                return NULL;

        memcpy (p, line + match->rm_so, len);
        p[len] = '\0';
        return p;
}

static int set_string_arg (char **var, const char *line, regmatch_t * match)
{
        char *arg = get_string_arg (line, match);

        if (!arg)
                return -1;

        if (*var != NULL) {
                safefree (*var);
        }

        *var = arg;

        return 0;
}

static int get_bool_arg (const char *line, regmatch_t * match)
{
        const char *p = line + match->rm_so;

        assert (line);
        assert (match && match->rm_so != -1);

        /* "y"es or o"n" map as true, otherwise it's false. */
        if (tolower (p[0]) == 'y' || tolower (p[1]) == 'n')
                return 1;
        else
                return 0;
}

static int
set_bool_arg (unsigned int *var, const char *line, regmatch_t * match)
{
        assert (var);
        assert (line);
        assert (match && match->rm_so != -1);

        *var = get_bool_arg (line, match);
        return 0;
}

static unsigned long
get_long_arg (const char *line, regmatch_t * match)
{
        assert (line);
        assert (match && match->rm_so != -1);

        return strtoul (line + match->rm_so, NULL, 0);
}

static int
set_int_arg (unsigned int *var, const char *line, regmatch_t * match)
{
        assert (var);
        assert (line);
        assert (match);

        *var = (unsigned int) get_long_arg (line, match);
        return 0;
}

/***********************************************************************
 *
 * Below are all the directive handling functions.  You will notice
 * that most of the directives delegate to one of the basic data
 * extraction routines.  This is deliberate.  To add a new directive
 * to tinyproxy only requires you to define the regular expression
 * above and then figure out what data extract routine to use.
 *
 * However, you will also notice that more complicated directives are
 * possible.  You can make your directive as complicated as you require
 * to express a solution to the problem you're tackling.
 *
 * See the definition/comment about the HANDLE_FUNC() macro to learn
 * what arguments are supplied to the handler, and to determine what
 * values to return.
 *
 ***********************************************************************/

static HANDLE_FUNC (handle_basicauthrealm)
{
        return set_string_arg (&conf->basicauth_realm, line, &match[2]);
}

static HANDLE_FUNC (handle_logfile)
{
        return set_string_arg (&conf->logf_name, line, &match[2]);
}

static HANDLE_FUNC (handle_pidfile)
{
        return set_string_arg (&conf->pidpath, line, &match[2]);
}

static HANDLE_FUNC (handle_anonymous)
{
        char *arg = get_string_arg (line, &match[2]);

        if (!arg)
                return -1;

        if(anonymous_insert (conf, arg) < 0) {
                CP_WARN ("anonymous_insert() failed: '%s'", arg);
                safefree(arg);
                return -1;
        }

        return 0;
}

static HANDLE_FUNC (handle_viaproxyname)
{
        int r = set_string_arg (&conf->via_proxy_name, line, &match[2]);

        if (r)
                return r;
        log_message (LOG_INFO,
                     "Setting \"Via\" header to '%s'",
                     conf->via_proxy_name);
        return 0;
}

static HANDLE_FUNC (handle_disableviaheader)
{
        int r = set_bool_arg (&conf->disable_viaheader, line, &match[2]);

        if (r) {
                return r;
        }

        log_message (LOG_INFO,
                     "Disabling transmission of the \"Via\" header.");
        return 0;
}

static HANDLE_FUNC (handle_defaulterrorfile)
{
        return set_string_arg (&conf->errorpage_undef, line, &match[2]);
}

static HANDLE_FUNC (handle_statfile)
{
        return set_string_arg (&conf->statpage, line, &match[2]);
}

static HANDLE_FUNC (handle_stathost)
{
        int r = set_string_arg (&conf->stathost, line, &match[2]);

        if (r)
                return r;
        log_message (LOG_INFO, "Stathost set to \"%s\"", conf->stathost);
        return 0;
}

static HANDLE_FUNC (handle_xtinyproxy)
{
#ifdef XTINYPROXY_ENABLE
        return set_bool_arg (&conf->add_xtinyproxy, line, &match[2]);
#else
        if(!get_bool_arg(line, &match[2]))
                return 0;
        fprintf (stderr,
                 "XTinyproxy NOT Enabled! Recompile with --enable-xtinyproxy\n");
        return 1;
#endif
}

static HANDLE_FUNC (handle_syslog)
{
        return set_bool_arg (&conf->syslog, line, &match[2]);
}

static HANDLE_FUNC (handle_bindsame)
{
        int r = set_bool_arg (&conf->bindsame, line, &match[2]);

        if (r)
                return r;
        log_message (LOG_INFO, "Binding outgoing connection to incoming IP");
        return 0;
}

static HANDLE_FUNC (handle_port)
{
        set_int_arg (&conf->port, line, &match[2]);

        if (conf->port > 65535) {
                fprintf (stderr, "Bad port number (%d) supplied for Port.\n",
                         conf->port);
                return 1;
        }

        return 0;
}

static HANDLE_FUNC (handle_maxclients)
{
        set_int_arg (&conf->maxclients, line, &match[2]);
        return 0;
}

static HANDLE_FUNC (handle_obsolete)
{
        fprintf (stderr, "WARNING: obsolete config item on line %lu\n",
                 lineno);
        return 0;
}

static HANDLE_FUNC (handle_timeout)
{
        return set_int_arg (&conf->idletimeout, line, &match[2]);
}

static HANDLE_FUNC (handle_connectport)
{
        add_connect_port_allowed (get_long_arg (line, &match[2]),
                                  &conf->connect_ports);
        return 0;
}

static HANDLE_FUNC (handle_user)
{
        return set_string_arg (&conf->user, line, &match[2]);
}

static HANDLE_FUNC (handle_group)
{
        return set_string_arg (&conf->group, line, &match[2]);
}

static void warn_invalid_address(char *arg, unsigned long lineno) {
        CP_WARN ("Invalid address %s", arg);
}

static HANDLE_FUNC (handle_allow)
{
        char *arg = get_string_arg (line, &match[2]);

        if(insert_acl (arg, ACL_ALLOW, &conf->access_list) < 0)
                warn_invalid_address (arg, lineno);
        safefree (arg);
        return 0;
}

static HANDLE_FUNC (handle_deny)
{
        char *arg = get_string_arg (line, &match[2]);

        if(insert_acl (arg, ACL_DENY, &conf->access_list) < 0)
                warn_invalid_address (arg, lineno);
        safefree (arg);
        return 0;
}

static HANDLE_FUNC (handle_bind)
{
        char *arg = get_string_arg (line, &match[2]);

        if (arg == NULL) {
                return -1;
        }

        if (conf->bind_addrs == NULL) {
               conf->bind_addrs = sblist_new(sizeof(char*), 16);
               if (conf->bind_addrs == NULL) {
                       CP_WARN ("Could not create a list "
                                   "of bind addresses.", "");
                       safefree(arg);
                       return -1;
               }
        }

        sblist_add (conf->bind_addrs, &arg);

        log_message (LOG_INFO,
                     "Added bind address [%s] for outgoing connections.", arg);

        return 0;
}

static HANDLE_FUNC (handle_listen)
{
        char *arg = get_string_arg (line, &match[2]);

        if (arg == NULL) {
                return -1;
        }

        if (conf->listen_addrs == NULL) {
               conf->listen_addrs = sblist_new(sizeof(char*), 16);
               if (conf->listen_addrs == NULL) {
                       CP_WARN ("Could not create a list "
                                   "of listen addresses.", "");
                       safefree(arg);
                       return -1;
               }
        }

        sblist_add (conf->listen_addrs, &arg);

        log_message(LOG_INFO, "Added address [%s] to listen addresses.", arg);

        return 0;
}

static HANDLE_FUNC (handle_errorfile)
{
        /*
         * Because an integer is defined as ((0x)?[[:digit:]]+) _two_
         * match places are used.  match[2] matches the full digit
         * string, while match[3] matches only the "0x" part if
         * present.  This is why the "string" is located at
         * match[4] (rather than the more intuitive match[3].
         */
        unsigned long int err = get_long_arg (line, &match[2]);
        char *page = get_string_arg (line, &match[4]);

        if(add_new_errorpage (conf, page, err) < 0) {
                CP_WARN ("add_new_errorpage() failed: '%s'", page);
                safefree (page);
        }
        return 0;
}

static HANDLE_FUNC (handle_addheader)
{
        char *name = get_string_arg (line, &match[2]);
        char *value = get_string_arg (line, &match[3]);
        http_header_t header;

        if (!conf->add_headers) {
                conf->add_headers = sblist_new (sizeof(http_header_t), 16);
        }

        header.name = name;
        header.value = value;

        sblist_add (conf->add_headers, &header);

        /* Don't free name or value here, as they are referenced in the
         * struct inserted into the vector. */

        return 0;
}

/*
 * Log level's strings.

 */
struct log_levels_s {
        const char *string;
        int level;
};
static struct log_levels_s log_levels[] = {
        {"critical", LOG_CRIT},
        {"error", LOG_ERR},
        {"warning", LOG_WARNING},
        {"notice", LOG_NOTICE},
        {"connect", LOG_CONN},
        {"info", LOG_INFO}
};

static HANDLE_FUNC (handle_loglevel)
{
        static const unsigned int nlevels =
            sizeof (log_levels) / sizeof (log_levels[0]);
        unsigned int i;

        char *arg = get_string_arg (line, &match[2]);

        for (i = 0; i != nlevels; ++i) {
                if (!strcasecmp (arg, log_levels[i].string)) {
                        set_log_level (log_levels[i].level);
                        safefree (arg);
                        return 0;
                }
        }

        safefree (arg);
        return -1;
}

static HANDLE_FUNC (handle_basicauth)
{
        char *user, *pass;
        user = get_string_arg(line, &match[2]);
        if (!user)
                return -1;
        pass = get_string_arg(line, &match[3]);
        if (!pass) {
                safefree (user);
                return -1;
        }
        if (!conf->basicauth_list) {
                conf->basicauth_list = sblist_new (sizeof(char*), 16);
        }

        basicauth_add (conf->basicauth_list, user, pass);
        safefree (user);
        safefree (pass);
        return 0;
}

#ifdef FILTER_ENABLE

static void warn_deprecated(const char *arg, unsigned long lineno) {
        CP_WARN ("deprecated option %s", arg);
}

static HANDLE_FUNC (handle_filter)
{
        return set_string_arg (&conf->filter, line, &match[2]);
}

static HANDLE_FUNC (handle_filterurls)
{
        conf->filter_opts |=
             get_bool_arg (line, &match[2]) * FILTER_OPT_URL;
        return 0;
}

static HANDLE_FUNC (handle_filterextended)
{
        warn_deprecated("FilterExtended, use FilterType", lineno);
        conf->filter_opts |=
             get_bool_arg (line, &match[2]) * FILTER_OPT_TYPE_ERE;
        return 0;
}

static HANDLE_FUNC (handle_filterdefaultdeny)
{
        assert (match[2].rm_so != -1);
        conf->filter_opts |=
            get_bool_arg (line, &match[2]) * FILTER_OPT_DEFAULT_DENY;
        return 0;
}

static HANDLE_FUNC (handle_filtercasesensitive)
{
        conf->filter_opts |=
            get_bool_arg (line, &match[2]) * FILTER_OPT_CASESENSITIVE;
        return 0;
}

static HANDLE_FUNC (handle_filtertype)
{
        static const struct { unsigned short flag; char type[8]; }
        ftmap[] = {
             {FILTER_OPT_TYPE_ERE,	"ere"},
             {FILTER_OPT_TYPE_BRE,	"bre"},
             {FILTER_OPT_TYPE_FNMATCH,	"fnmatch"},
        };
        char *type;
        unsigned i;
        type = get_string_arg(line, &match[2]);
        if (!type) return -1;

        for(i=0;i<sizeof(ftmap)/sizeof(ftmap[0]);++i)
                if(!strcasecmp(ftmap[i].type, type))
                        conf->filter_opts |= ftmap[i].flag;

        safefree (type);
        return 0;
}
#endif

#ifdef REVERSE_SUPPORT
static HANDLE_FUNC (handle_reverseonly)
{
        return set_bool_arg (&conf->reverseonly, line, &match[2]);
}

static HANDLE_FUNC (handle_reversemagic)
{
        return set_bool_arg (&conf->reversemagic, line, &match[2]);
}

static HANDLE_FUNC (handle_reversebaseurl)
{
        return set_string_arg (&conf->reversebaseurl, line, &match[2]);
}

static HANDLE_FUNC (handle_reversepath)
{
        /*
         * The second string argument is optional.
         */
        char *arg1, *arg2;

        arg1 = get_string_arg (line, &match[2]);
        if (!arg1)
                return -1;

        if (match[4].rm_so != -1) {
                arg2 = get_string_arg (line, &match[4]);
                if (!arg2) {
                        safefree (arg1);
                        return -1;
                }
                reversepath_add (arg1, arg2, &conf->reversepath_list);
                safefree (arg1);
                safefree (arg2);
        } else {
                reversepath_add (NULL, arg1, &conf->reversepath_list);
                safefree (arg1);
        }
        return 0;
}
#endif

#ifdef UPSTREAM_SUPPORT

static enum proxy_type pt_from_string(const char *s)
{
	static const char pt_map[][7] = {
		[PT_NONE]   = "none",
		[PT_HTTP]   = "http",
		[PT_SOCKS4] = "socks4",
		[PT_SOCKS5] = "socks5",
	};
	unsigned i;
	for (i = 0; i < sizeof(pt_map)/sizeof(pt_map[0]); i++)
		if (!strcmp(pt_map[i], s))
			return i;
	return PT_NONE;
}

static HANDLE_FUNC (handle_upstream)
{
        char *ip;
        int port, mi;
        char *domain = 0, *user = 0, *pass = 0, *tmp;
        enum proxy_type pt;
        enum upstream_build_error ube;

        if (match[3].rm_so != -1) {
                tmp = get_string_arg (line, &match[3]);
                if(!strcmp(tmp, "none")) {
                        safefree(tmp);
                        if (match[4].rm_so == -1) return -1;
                        domain = get_string_arg (line, &match[4]);
                        if (!domain)
                                return -1;
                        ube = upstream_add (NULL, 0, domain, 0, 0, PT_NONE, &conf->upstream_list);
                        safefree (domain);
                        goto check_err;
                }
        }

        mi = 6;

        tmp = get_string_arg (line, &match[mi]);
        pt = pt_from_string(tmp);
        safefree(tmp);
        mi += 2;

        if (match[mi].rm_so != -1)
                user = get_string_arg (line, &match[mi]);
        mi++;

	if (match[mi].rm_so != -1)
                pass = get_string_arg (line, &match[mi]);
        mi++;

        if (match[mi+4].rm_so != -1) /* IPv6 address in square brackets */
                ip = get_string_arg (line, &match[mi+4]);
        else
                ip = get_string_arg (line, &match[mi]);
        if (!ip)
                return -1;
        mi += 16;

        port = (int) get_long_arg (line, &match[mi]);
        mi += 3;

        if (match[mi].rm_so != -1)
                domain = get_string_arg (line, &match[mi]);

        ube = upstream_add (ip, port, domain, user, pass, pt, &conf->upstream_list);

        safefree (user);
        safefree (pass);
        safefree (domain);
        safefree (ip);

check_err:;
        if(ube != UBE_SUCCESS)
                CP_WARN("%s", upstream_build_error_string(ube));
        return 0;
}

#endif
