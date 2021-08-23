#include "common.h"
#include "hostspec.h"
#include "heap.h"
#include "network.h"

static int dotted_mask(char *bitmask_string, unsigned char array[])
{
	unsigned char v4bits[4];
	if (1 != inet_pton (AF_INET, bitmask_string, v4bits)) return -1;
	memset (array, 0xff, IPV6_LEN-4);
	memcpy (array + IPV6_LEN-4, v4bits, 4);
	return 0;
}

/*
 * Fills in the netmask array given a numeric value.
 *
 * Returns:
 *   0 on success
 *  -1 on failure (invalid mask value)
 *
 */
static int
fill_netmask_array (char *bitmask_string, int v6,
		    unsigned char array[])
{
	unsigned int i;
	unsigned long int mask;
	char *endptr;

	errno = 0;              /* to distinguish success/failure after call */
	if (strchr (bitmask_string, '.')) {
		if (v6) return -1; /* ipv6 doesn't supported dotted netmasks */
		return dotted_mask(bitmask_string, array);
	}
	mask = strtoul (bitmask_string, &endptr, 10);

	/* check for various conversion errors */
	if ((errno == ERANGE && mask == ULONG_MAX)
	    || (errno != 0 && mask == 0) || (endptr == bitmask_string))
		return -1;

	if (v6 == 0) {
		/* The mask comparison is done as an IPv6 address, so
		 * convert to a longer mask in the case of IPv4
		 * addresses. */
		mask += 12 * 8;
	}

	/* check valid range for a bit mask */
	if (mask > (8 * IPV6_LEN))
		return -1;

	/* we have a valid range to fill in the array */
	for (i = 0; i != IPV6_LEN; ++i) {
		if (mask >= 8) {
			array[i] = 0xff;
			mask -= 8;
		} else if (mask > 0) {
			array[i] = (unsigned char) (0xff << (8 - mask));
			mask = 0;
		} else {
			array[i] = 0;
		}
	}

	return 0;
}


/* parse a location string containing either an ipv4/ipv4 + hostmask tuple
   or a dnsname into a struct hostspec.
   returns 0 on success, non-0 on error (might be memory allocation, bogus
   ip address or mask).
*/
int hostspec_parse(char *location, struct hostspec *h) {
	char *mask, ip_dst[IPV6_LEN];

	h->type = HST_NONE;
	if(!location) return 0;

	memset(h, 0, sizeof(*h));
	if ((mask = strrchr(location, '/')))
		*(mask++) = 0;

	/*
	 * Check for a valid IP address (the simplest case) first.
	 */
	if (full_inet_pton (location, ip_dst) > 0) {
		h->type = HST_NUMERIC;
		memcpy (h->address.ip.network, ip_dst, IPV6_LEN);
		if(!mask) memset (h->address.ip.mask, 0xff, IPV6_LEN);
		else {
			char dst[sizeof(struct in6_addr)];
			int v6, i;
			/* Check if the IP address before the netmask is
			 * an IPv6 address */
			if (inet_pton(AF_INET6, location, dst) > 0)
				v6 = 1;
			else
				v6 = 0;

			if (fill_netmask_array
			    (mask, v6, &(h->address.ip.mask[0]))
			     < 0)
				goto err;

			for (i = 0; i < IPV6_LEN; i++)
				h->address.ip.network[i] = ip_dst[i] &
				h->address.ip.mask[i];
		}
	} else {
		/* either bogus IP or hostname */
			/* bogus ipv6 ? */
		if (mask || strchr (location, ':'))
			goto err;

		/* In all likelihood a string */
		h->type = HST_STRING;
		h->address.string = safestrdup (location);
		if (!h->address.string)
			goto err;
	}
	/* restore mask */
	if(mask) *(--mask) = '/';
	return 0;
err:;
	if(mask) *(--mask) = '/';
	return -1;
}

static int string_match(const char *ip, const char *addrspec)
{
	size_t test_length, match_length;
	if(!strcasecmp(ip, addrspec)) return 1;
	if(addrspec[0] != '.') return 0;
	test_length = strlen (ip);
	match_length = strlen (addrspec);
	if (test_length < match_length) return 0;
	return (strcasecmp
	    (ip + (test_length - match_length),
	     addrspec) == 0);
}

static int numeric_match(const uint8_t addr[], const struct hostspec *h)
{
	uint8_t x, y;
	int i;

	for (i = 0; i != IPV6_LEN; ++i) {
		x = addr[i] & h->address.ip.mask[i];
		y = h->address.ip.network[i];

		/* If x and y don't match, the IP addresses don't match */
		if (x != y)
			return 0;
	}

	return 1;
}

/* check whether ip matches hostspec.
   return 1 on match, 0 on non-match */
int hostspec_match(const char *ip, const struct hostspec *h) {
	int is_numeric_addr;
	uint8_t numeric_addr[IPV6_LEN];
	if (ip[0] == '\0') return 0;
	is_numeric_addr = (full_inet_pton (ip, &numeric_addr) > 0);
	switch (h->type) {
	case HST_STRING:
		if(is_numeric_addr) return 0;
		return string_match (ip, h->address.string);
	case HST_NUMERIC:
		return numeric_match (numeric_addr, h);
	case HST_NONE:
		return 0;
	}
	return 0;
}
