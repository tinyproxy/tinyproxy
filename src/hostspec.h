#ifndef HOSTSPEC_H
#define HOSTSPEC_H

#define IPV6_LEN 16

enum hostspec_type {
	HST_NONE,
	HST_STRING,
	HST_NUMERIC,
};

struct hostspec {
	enum hostspec_type type;
	union {
                char *string;
                struct {
                        unsigned char network[IPV6_LEN];
                        unsigned char mask[IPV6_LEN];
                } ip;
	} address;
};

int hostspec_parse(char *domain, struct hostspec *h);
int hostspec_match(const char *ip, const struct hostspec *h);

#endif
