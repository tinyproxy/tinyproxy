/* $Id: dnsclient.h,v 1.1 2002-05-23 04:40:06 rjkaes Exp $
 *
 * See 'dnsclient.c' for a detailed description.
 *
 * Copyright (C) 2002  Robert James Kaes (rjkaes@flarenet.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef TINYPROXY_DNSCLIENT_H
#define TINYPROXY_DNSCLIENT_H

extern int start_dnsserver(const char* dnsserver_location, const char* path);
extern int stop_dnsserver(void);

extern int dns_connect(void);
extern void dns_disconnect(int dns);

extern int dns_gethostbyaddr(int dns, const char* ipaddr, char** hostname,
			     size_t hostlen);
extern int dns_getaddrbyname(int dns, const char* name,
			     struct in_addr** addresses);

#endif
