/* $Id: ternary.h,v 1.4 2001-11-22 00:31:10 rjkaes Exp $
 *
 * See 'ternary.c' for a detailed description.
 *
 * Copyright (C) 2000  Robert James Kaes (rjkaes@flarenet.com)
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

#ifndef _TINYPROXY_TERNARY_H_
#define _TINYPROXY_TERNARY_H_

/*
 * Holds our token for a ternary tree.
 */
typedef long int TERNARY;

/*
 * Macros for testing for errors from the various functions.
 */
#define TE_ISERROR(x)	((x) < 0)	/* true if x is tlib error code */
#define TE_NONE		0	/* no errors */

/*
 * Contains any error messages from the functions.
 */
extern char te_errbuf[256];

/*
 * Macros to fill in te_errbuf
 */
#define ERRBUF(str)  		strncpy(te_errbuf, str, sizeof(te_errbuf))
#define ERRBUF2(str,n)		sprintf(te_errbuf, str, n)
#define ERRBUF3(str,n,m)	sprintf(te_errbuf, str, n, m)

/*
 * Error return codes
 */
#define TE_BADTOKEN	-3	/* back token for the trees */
#define TE_EMPTY	-4	/* there is no data found */
#define TE_TOOFULL	-5	/* the buffers are filled */
#define TE_NOROOM	-6	/* can't allocate space (sys err) */
#define TE_TOOMANYTS	-7	/* too many trees in use */
#define TE_INTINCON	-8	/* internal inconsistency */
#define TE_EXISTS	-9	/* key already exists in tree */

/*
 * Library functions.
 */
extern TERNARY ternary_new(void);
extern int ternary_destroy(TERNARY tno, void (*freeptr) (void *));

#define ternary_insert(x, y, z)  ternary_insert_replace(x, y, z, 0)
#define ternary_replace(x, y, z) ternary_insert_replace(x, y, z, 1)

extern int ternary_insert_replace(TERNARY tno, const char *s, void *data,
				  short int replace);
extern int ternary_search(TERNARY tno, const char *s, void **data);

#endif
