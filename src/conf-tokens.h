#ifndef CONF_TOKENS_H
#define CONF_TOKENS_H

enum config_directive {
CD_NIL = 0,
CD_logfile,
CD_pidfile,
CD_anonymous,
CD_viaproxyname,
CD_defaulterrorfile,
CD_statfile,
CD_stathost,
CD_xtinyproxy,
CD_syslog,
CD_bindsame,
CD_disableviaheader,
CD_port,
CD_maxclients,
CD_maxspareservers,
CD_minspareservers,
CD_startservers,
CD_maxrequestsperchild,
CD_timeout,
CD_connectport,
CD_user,
CD_group,
CD_listen,
CD_allow,
CD_deny,
CD_bind,
CD_basicauth,
CD_basicauthrealm,
CD_errorfile,
CD_addheader,
CD_filter,
CD_filterurls,
CD_filtertype,
CD_filterextended,
CD_filterdefaultdeny,
CD_filtercasesensitive,
CD_reversebaseurl,
CD_reverseonly,
CD_reversemagic,
CD_reversepath,
CD_upstream,
CD_loglevel,
};

struct config_directive_entry { const char* name; enum config_directive value; };

const struct config_directive_entry *
config_directive_find (register const char *str, register size_t len);

#endif

