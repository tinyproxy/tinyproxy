/* $Id: text.h,v 1.2 2003-03-13 05:20:06 rjkaes Exp $
 *
 * See 'text.c' for a detailed description.
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

#ifndef TINYPROXY_TEXT_H
#define TINYPROXY_TEXT_H

#ifndef HAVE_STRLCAT
extern size_t strlcat(char *dst, const char *src, size_t size);
#endif				/* HAVE_STRLCAT */

#ifndef HAVE_STRLCPY
extern size_t strlcpy(char *dst, const char *src, size_t size);
#endif				/* HAVE_STRLCPY */

extern ssize_t chomp(char *buffer, size_t length);

#endif
