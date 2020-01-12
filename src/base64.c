/* tinyproxy - A fast light-weight HTTP proxy
 * this file Copyright (C) 2016-2018 rofl0r
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

#include "base64.h"

static const char base64_tbl[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
   rofl0r's base64 impl (taken from libulz)
   takes count bytes from src, writing base64 encoded string into dst.
   dst needs to be at least BASE64ENC_BYTES(count) + 1 bytes in size.
   the string in dst will be zero-terminated.
   */
void base64enc(char *dst, const void* src, size_t count)
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

