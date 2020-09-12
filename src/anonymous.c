/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2000 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* Handles insertion and searches for headers which should be let through
 * when the anonymous feature is turned on.
 */

#include "main.h"

#include "anonymous.h"
#include "hsearch.h"
#include "heap.h"
#include "log.h"
#include "conf.h"

short int is_anonymous_enabled (struct config_s *conf)
{
        return (conf->anonymous_map != NULL) ? 1 : 0;
}

/*
 * Search for the header.  This function returns a positive value greater than
 * zero if the string was found, zero if it wasn't and negative upon error.
 */
int anonymous_search (struct config_s *conf, const char *s)
{
        assert (s != NULL);
        assert (conf->anonymous_map != NULL);

        return !!htab_find (conf->anonymous_map, s);
}

/*
 * Insert a new header.
 *
 * Return -1 if there is an error, otherwise a 0 is returned if the insert was
 * successful.
 */
int anonymous_insert (struct config_s *conf, char *s)
{
        assert (s != NULL);

        if (!conf->anonymous_map) {
                conf->anonymous_map = htab_create (32);
                if (!conf->anonymous_map)
                        return -1;
        }

        if (htab_find (conf->anonymous_map, s)) {
                /* The key was already found. */
                return 0;
        }

        /* Insert the new key */
        return htab_insert (conf->anonymous_map, s, HTV_N(1)) ? 0 : -1;
}
