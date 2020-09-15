#include <pthread.h>
#include <time.h>
#include "loop.h"
#include "conf.h"
#include "main.h"
#include "sblist.h"
#include "sock.h"

struct loop_record {
	union sockaddr_union addr;
	time_t tstamp;
};

static sblist *loop_records;
static pthread_mutex_t loop_records_lock = PTHREAD_MUTEX_INITIALIZER;

void loop_records_init(void) {
	loop_records = sblist_new(sizeof (struct loop_record), 32);
}

void loop_records_destroy(void) {
	sblist_free(loop_records);
	loop_records = 0;
}

#if 0
static void su_to_str(union sockaddr_union *addr, char *buf) {
	int af = addr->v4.sin_family;
	unsigned port = ntohs(af == AF_INET ? addr->v4.sin_port : addr->v6.sin6_port);
	char portb[32];
	sprintf(portb, ":%u", port);
        getpeer_information (addr, buf, 256);
	strcat(buf, portb);
}
#endif

void loop_records_add(union sockaddr_union *addr) {
	time_t now =time(0);
	struct loop_record rec;
	pthread_mutex_lock(&loop_records_lock);
	rec.tstamp = now;
	rec.addr = *addr;
	sblist_add(loop_records, &rec);
	pthread_mutex_unlock(&loop_records_lock);
}

#define TIMEOUT_SECS 15

int connection_loops (union sockaddr_union *addr) {
	int ret = 0, af, our_af = addr->v4.sin_family;
	void *ipdata, *our_ipdata = our_af == AF_INET ? (void*)&addr->v4.sin_addr.s_addr : (void*)&addr->v6.sin6_addr.s6_addr;
	size_t i, cmp_len = our_af == AF_INET ? sizeof(addr->v4.sin_addr.s_addr) : sizeof(addr->v6.sin6_addr.s6_addr);
	unsigned port, our_port = ntohs(our_af == AF_INET ? addr->v4.sin_port : addr->v6.sin6_port);
	time_t now = time(0);

	pthread_mutex_lock(&loop_records_lock);
	for (i = 0; i < sblist_getsize(loop_records); ) {
		struct loop_record *rec = sblist_get(loop_records, i);

		if (rec->tstamp + TIMEOUT_SECS < now) {
			sblist_delete(loop_records, i);
			continue;
		}

		if (!ret) {
			af = rec->addr.v4.sin_family;
			if (af != our_af) goto next;
			port = ntohs(af == AF_INET ? rec->addr.v4.sin_port : rec->addr.v6.sin6_port);
			if (port != our_port) goto next;
			ipdata = af == AF_INET ? (void*)&rec->addr.v4.sin_addr.s_addr : (void*)&rec->addr.v6.sin6_addr.s6_addr;
			if (!memcmp(ipdata, our_ipdata, cmp_len)) {
				ret = 1;
			}
		}
next:
		i++;
	}
	pthread_mutex_unlock(&loop_records_lock);
	return ret;
}

