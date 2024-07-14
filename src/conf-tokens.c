#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include "conf-tokens.h"

#ifdef HAVE_GPERF
#include "conf-tokens-gperf.inc"
#else

#include <strings.h>

const struct config_directive_entry *
config_directive_find (register const char *str, register size_t len)
{
  size_t i;
  static const struct config_directive_entry wordlist[] =
    {
      {"",CD_NIL}, {"",CD_NIL},
      {"allow", CD_allow},
      {"stathost", CD_stathost},
      {"listen", CD_listen},
      {"timeout", CD_timeout},
      {"statfile", CD_statfile},
      {"pidfile", CD_pidfile},
      {"bindsame", CD_bindsame},
      {"reversebaseurl", CD_reversebaseurl},
      {"viaproxyname", CD_viaproxyname},
      {"upstream", CD_upstream},
      {"anonymous", CD_anonymous},
      {"group", CD_group},
      {"defaulterrorfile", CD_defaulterrorfile},
      {"startservers", CD_startservers},
      {"filtercasesensitive", CD_filtercasesensitive},
      {"filtertype", CD_filtertype},
      {"filterurls", CD_filterurls},
      {"filter", CD_filter},
      {"reversemagic", CD_reversemagic},
      {"errorfile", CD_errorfile},
      {"minspareservers", CD_minspareservers},
      {"user", CD_user},
      {"disableviaheader", CD_disableviaheader},
      {"deny", CD_deny},
      {"xtinyproxy", CD_xtinyproxy},
      {"reversepath", CD_reversepath},
      {"bind", CD_bind},
      {"maxclients", CD_maxclients},
      {"reverseonly", CD_reverseonly},
      {"port", CD_port},
      {"maxspareservers", CD_maxspareservers},
      {"syslog", CD_syslog},
      {"filterdefaultdeny", CD_filterdefaultdeny},
      {"loglevel", CD_loglevel},
      {"filterextended", CD_filterextended},
      {"connectport", CD_connectport},
      {"logfile", CD_logfile},
      {"basicauth", CD_basicauth},
      {"basicauthrealm", CD_basicauthrealm},
      {"addheader", CD_addheader},
      {"maxrequestsperchild", CD_maxrequestsperchild}
    };

	for(i=0;i<sizeof(wordlist)/sizeof(wordlist[0]);++i) {
		if(!strcasecmp(str, wordlist[i].name))
			return &wordlist[i];
	}
	return 0;
}

#endif
