#include "common.h"
#include "cap.h"
#include "log.h"
#include <err.h>

void cap_begin(void) {
  cap_in = cap_init();
  if (cap_in == NULL)
		err(1, "Failed to communicate with libcasper");

  if (caph_limit_stdio() == -1)
    err(1, "Failed to limit stdio");
}

void cap_server_socket(int fd) {
  cap_rights_init(&rights, CAP_ACCEPT, CAP_EVENT, CAP_READ, CAP_WRITE);
  if (caph_rights_limit(fd, &rights) == -1)
	  err(1, "Unable to apply rights for sandbox");
}

void cap_start(bool syslog) {
	if (caph_enter_casper() == -1)
		err(1, "Failed to enter capability mode");

  log_message(LOG_NOTICE, "entered capability mode");

  cap_net = cap_service_open(cap_in, "system.net");
	if (cap_net == NULL)
		err(1, "Failed to open the system.net libcasper service");

  if (syslog) {
    cap_log = cap_service_open(cap_in, "system.syslog");
    if (cap_log == NULL)
      err(1, "Failed to open the system.syslog libcasper service");
  }

  cap_close(cap_in);

	limit = cap_net_limit_init(cap_net, CAPNET_NAME2ADDR |  CAPNET_CONNECT);
  if (limit == NULL)
		err(1, "Failed to create system.net limits");

  if (cap_net_limit(limit) == -1)
		err(1, "Failed to apply system.net limits");
}

