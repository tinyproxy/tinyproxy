/* tinyproxy - A fast light-weight HTTP proxy
 * This file: Copyright (C) 2016-2017 rofl0r
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "main.h"
#include "basicauth.h"

#include "conns.h"
#include "heap.h"
#include "html-error.h"
#include "log.h"
#include "conf.h"

/* calculates number of bytes base64-encoded stream of N bytes will take. */
#define BASE64ENC_BYTES(N) (((N+2)/3)*4)

static const char base64_tbl[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* my own base64 impl (taken from libulz) */
static void base64enc(char *dst, const void* src, size_t count)
{
	unsigned const char *s = src;
	char* d = dst;
	while(count) {
		int i = 0,  n = *s << 16;
		s++;
		count--;
		if(count) {
			n |= *s << 8;
			s++;
			count--;
			i++;
		}
		if(count) {
			n |= *s;
			s++;
			count--;
			i++;
		}
		*d++ = base64_tbl[(n >> 18) & 0x3f];
		*d++ = base64_tbl[(n >> 12) & 0x3f];
		*d++ = i ? base64_tbl[(n >> 6) & 0x3f] : '=';
		*d++ = i == 2 ? base64_tbl[n & 0x3f] : '=';
	}
	*d = 0;
}

/*
 * Add entry to the basicauth list
 */
void basicauth_add (vector_t authlist,
	const char *user, const char *pass)
{
	char tmp[256+2];
	char b[BASE64ENC_BYTES((sizeof tmp)-1) + 1];
	int l;
	size_t bl;

        if (user == NULL || pass == NULL) {
                log_message (LOG_WARNING,
                             "Illegal basicauth rule: missing user or pass");
                return;
        }

        l = snprintf(tmp, sizeof tmp, "%s:%s", user, pass);

        if(l >= (ssize_t) sizeof tmp) {
                log_message (LOG_WARNING,
                            "User / pass in basicauth rule too long");
                return;
        }

        base64enc(b, tmp, l);
        bl = BASE64ENC_BYTES(l) + 1;

        if (vector_append(authlist, b, bl) == -ENOMEM) {
                log_message (LOG_ERR,
                             "Unable to allocate memory in basicauth_add()");
                return;
        }

        log_message (LOG_INFO,
                     "Added basic auth user : %s", user);
}

/*
 * Check if a user/password combination (encoded as base64)
 * is in the basicauth list.
 * return 1 on success, 0 on failure.
 */
int basicauth_check (vector_t authlist, const char *authstring)
{
        ssize_t vl, i;
        size_t al, el;
        const char* entry;

        vl = vector_length (authlist);
        if (vl == -EINVAL) return 0;

        al = strlen (authstring);
        for (i = 0; i < vl; i++) {
                entry = vector_getentry (authlist, i, &el);
                if (strncmp (authstring, entry, al) == 0)
                        return 1;
        }
	return 0;
}
