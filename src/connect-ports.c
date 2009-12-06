/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1998 Steven Young <sdyoung@miranda.org>
 * Copyright (C) 1999-2005 Robert James Kaes <rjkaes@users.sourceforge.net>
 * Copyright (C) 2009 Michael Adam <obnox@samba.org>
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

#include "connect-ports.h"
#include "vector.h"
#include "log.h"

/*
 * This is a global variable which stores which ports are allowed by
 * the CONNECT method.  It's a security thing.
 */
static vector_t ports_allowed_by_connect = NULL;

/*
 * Now, this routine adds a "port" to the list.  It also creates the list if
 * it hasn't already by done.
 */
void add_connect_port_allowed (int port)
{
        if (!ports_allowed_by_connect) {
                ports_allowed_by_connect = vector_create ();
                if (!ports_allowed_by_connect) {
                        log_message (LOG_WARNING,
                                     "Could not create a list of allowed CONNECT ports");
                        return;
                }
        }

        log_message (LOG_INFO,
                     "Adding Port [%d] to the list allowed by CONNECT", port);
        vector_append (ports_allowed_by_connect, (void **) &port,
                       sizeof (port));
}

/*
 * This routine checks to see if a port is allowed in the CONNECT method.
 *
 * Returns: 1 if allowed
 *          0 if denied
 */
int check_allowed_connect_ports (int port)
{
        size_t i;
        int *data;

        /*
         * A port list is REQUIRED for a CONNECT request to function
         * properly.  This closes a potential security hole.
         */
        if (!ports_allowed_by_connect)
                return 0;

        for (i = 0; i != (size_t) vector_length (ports_allowed_by_connect); ++i) {
                data =
                    (int *) vector_getentry (ports_allowed_by_connect, i, NULL);
                if (data && *data == port)
                        return 1;
        }

        return 0;
}
