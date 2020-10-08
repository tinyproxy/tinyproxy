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
#define SPACE "[ \\t]"
#define WS SPACE "+"
#define STR "\"([^\"]+)\""
#define BOOL "(yes|on|no|off)"
#define INT "(()" DIGIT "+)"
#define ALNUM "([-a-z0-9._]+)"
#define USERNAME "([^:]*)"
#define PASSWORD "([^@]*)"
#define IP "((([0-9]{1,3})\\.){3}[0-9]{1,3})"
#define IPMASK "(" IP "(/" DIGIT "+)?)"
#define IPV6 "(" \
        "(([0-9a-f:]{2,39}))|" \
        "(([0-9a-f:]{0,29}:" IP "))" \
        ")"

#define IPV6MASK "(" IPV6 "(/" DIGIT "+)?)"
#define BEGIN "^" SPACE "*"
#define END SPACE "*$"


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
STDCONF (basicauth, ALNUM WS ALNUM, handle_basicauth),
STDCONF (errorfile, INT WS STR, handle_errorfile),
STDCONF (addheader,  STR WS STR, handle_addheader),

#ifdef FILTER_ENABLE
/* filtering */
STDCONF (filter, STR, handle_filter),
STDCONF (filterurls, BOOL, handle_filterurls),
STDCONF (filterextended, BOOL, handle_filterextended),
STDCONF (filterdefaultdeny, BOOL, handle_filterdefaultdeny),
STDCONF (filtercasesensitive, BOOL, handle_filtercasesensitive),
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
     "(" IP "|" ALNUM ")"
     ":" INT "(" WS STR ")?" ")", handle_upstream),
#endif
/* loglevel */
STDCONF (loglevel, "(critical|error|warning|notice|connect|info)",
 handle_loglevel)

