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
#include "base64.h"

/*
 * Create basic-auth token in buf.
 * Returns strlen of token on success,
 * -1 if user/pass missing
 * 0 if user/pass too long
 */
ssize_t basicauth_string(const char *user, const char *pass,
	char *buf, size_t bufsize)
{
	char tmp[256+2];
	int l;
	if (!user || !pass) return -1;
	l = snprintf(tmp, sizeof tmp, "%s:%s", user, pass);
	if (l < 0 || l >= (ssize_t) sizeof tmp) return 0;
	if (bufsize < (BASE64ENC_BYTES((unsigned)l) + 1)) return 0;
	base64enc(buf, tmp, l);
	return BASE64ENC_BYTES(l);
}

/*
 * Add entry to the basicauth list
 */
void basicauth_add (sblist *authlist,
	const char *user, const char *pass)
{
	char b[BASE64ENC_BYTES((256+2)-1) + 1], *s;
	ssize_t ret;

        ret = basicauth_string(user, pass, b, sizeof b);
        if (ret == -1) {
                log_message (LOG_WARNING,
                             "Illegal basicauth rule: missing user or pass");
                return;
        } else if (ret == 0) {
                log_message (LOG_WARNING,
                             "User / pass in basicauth rule too long");
                return;
        }

        if (!(s = safestrdup(b)) || !sblist_add(authlist, &s)) {
                safefree(s);
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
int basicauth_check (sblist *authlist, const char *authstring)
{
        size_t i;
        char** entry;

        if (!authlist) return 0;

        for (i = 0; i < sblist_getsize(authlist); i++) {
                entry = sblist_get (authlist, i);
                if (entry && strcmp (authstring, *entry) == 0)
                        return 1;
        }
	return 0;
}
