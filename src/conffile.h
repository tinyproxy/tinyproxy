/* $Id: conffile.h,v 1.2 2005-08-15 03:54:31 rjkaes Exp $
 *
 * See 'conffile.c' for more details.
 *
 * Copyright (C) 2004  Robert James Kaes (rjkaes@users.sourceforge.net)
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

#ifndef TINYPROXY_CONFFILE_H
#define TINYPROXY_CONFFILE_H

extern int config_compile(void);
extern int config_parse(struct config_s *conf, FILE * f);

#endif
