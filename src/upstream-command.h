#ifndef _TINYPROXY_UPSTREAM_COMMAND_H_
#define _TINYPROXY_UPSTREAM_COMMAND_H_

#include <stdio.h>

typedef enum upstream_command_result {
  UPSTREAM_DIRECT = 1,
  UPSTREAM_FALLTHROUGH,
  UPSTREAM_PROXY,
} upstream_command_result;

struct upstream_command_state {
        int pid;
        FILE *in;
        FILE *out;
};


#endif /* _TINYPROXY_UPSTREAM_COMMAND_H_ */
