/* $Id: utils.h,v 1.6 2001-08-30 16:52:56 rjkaes Exp $
 *
 * See 'utils.h' for a detailed description.
 *
 * Copyright (C) 1998  Steven Young
 * Copyright (C) 1999  Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef _TINYPROXY_UTILS_H_
#define _TINYPROXY_UTILS_H_

#include "tinyproxy.h"

extern int httperr(struct conn_s *connptr, int err, const char *msg);

extern void makedaemon(void);
extern void pidfile_create(const char *path);

#ifndef HAVE_STRLCAT
extern size_t strlcat(char *dst, const char *src, size_t size);
#endif /* HAVE_STRLCAT */

#ifndef HAVE_STRLCPY
extern size_t strlcpy(char *dst, const char *src, size_t size);
#endif /* HAVE_STRLCPY */

#endif
