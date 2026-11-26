#include "common.h"

#ifdef WITH_CASPER

/* hack to make capsicum work */
#ifndef inline
#define inline
#endif

#include <capsicum_helpers.h>

#include <sys/nv.h>
#include <sys/capsicum.h>

#include <libcasper.h>
#include <casper/cap_net.h>
#include <casper/cap_syslog.h>

extern cap_rights_t rights;
extern cap_channel_t *cap_net, *cap_log, *cap_in;
extern cap_net_limit_t *limit;

void cap_begin(void);
void cap_server_socket(int fd);
void cap_start(bool syslog);

#endif /* WITH_CASPER */

