#ifndef LOOP_H
#define LOOP_H

#include "sock.h"

void loop_records_init(void);
void loop_records_destroy(void);
void loop_records_add(union sockaddr_union *addr);
int connection_loops (union sockaddr_union *addr);

#endif

