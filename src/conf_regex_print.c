/* this is a tool to print regexname regex pairs as input for re2r.
   compile with gcc -I. src/conf_regex_print.c
 */

#include "config.h"

#include <stdio.h>

#define STDCONF(A, B, C) printf("%s %s\n", #A, B)

int main() {
#include "conf_regex.h"
;
}
