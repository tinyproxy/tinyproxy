/* $Id: ternary.c,v 1.5 2001-05-23 17:59:53 rjkaes Exp $
 *
 * This module creates a Ternary Search Tree which can store both string
 * keys, and arbitrary data for each key. It works similar to a hash, and
 * a binary search tree. The idea (and some code) is taken from Dr. Dobb's
 * Journal, April 1998 "Ternary Search Trees", Jon Bentley and Bob
 * Sedgewick, pg 20-25. The supplied code was then made "production" ready.
 * Hopefully, the comments in the code should make the API self
 * explanatory.
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

#if defined HAVE_CONFIG_H
#  include 	<config.h>
#endif

#if defined HAVE_SYS_TYPES_H
#  include 	<sys/types.h>
#endif

#include 	<stdio.h>
#include 	<stdlib.h>
#include	<string.h>

#include	"log.h"
#include 	"ternary.h"
#include	"tinyproxy.h"

/*
 * Macros for the tree structures (limits)
 */
#define MAXTREES	1024	/* max. number of trees */

/*
 * The structure for each individual node of the ternary tree.
 */
typedef struct tnode {
	char splitchar;
	struct tnode *lokid, *eqkid, *hikid;
} Tnode;

/*
 * The structure for each root of a ternary tree.
 */
#define BUFSIZE 1000
#define BUFARRAY 10000
typedef struct ttree {
	TERNARY token;		/* contains unique ternary tree ID */
	Tnode *tree_root;
	Tnode *buf;
	int bufn, freen;
	Tnode *freearr[BUFARRAY];
} Ttree;

/*
 * variables shared by library routines.
 */
static Ttree *trees[MAXTREES];	/* the array of trees */

/*
 * nonce generator -- this MUST be non-zero _always_
 */
#define IOFFSET	0x1221	/* used to hide index number in token */
#define NOFFSET 0x0502	/* initial nonce */
static unsigned int noncectr = NOFFSET;

/*
 * Contains any error messages from the functions.
 */
char te_errbuf[256];

/*
 * Generate a token; this is an integer: index number + OFFSET,,none
 * WARNING: Each quantity must fix into 16 bits
 *
 * Parameters:	int index	index number
 * Returned:	TERNARY		token of corresponding tree
 * Errors:	TE_INTINCON	* index + OFFSET is too big
 *				* nonce is too big
 *				* index is out of range
 *				(te_errbuf has disambiguating string)
 * Exceptions:	none
 */
static TERNARY create_token_ref(unsigned int ind)
{
	unsigned int high;     	/* high 16 bits of token (index) */
	unsigned int low;      	/* low 16 bits of token (nonce) */

	/*
	 * Sanity check argument; called internally...
	 */
	if (ind > MAXTREES) {
		ERRBUF3("create_token_ref: index %u exceeds %d", ind, MAXTREES);
		return TE_INTINCON;
	}

	/*
	 * Get the high part of the token (index into tree array, offset by
	 * some arbitrary amount)
	 * SANITY CHECK: Be sure index + OFFSET fits into 16 bits as positive.
	 */
	high = (ind + IOFFSET) & 0x7fff;
	if (high != ind + IOFFSET) {
		ERRBUF3("create_token_ref: index %u larger than %u", ind,
			0x7fff - IOFFSET);
		return TE_INTINCON;
	}

	/*
	 * Get the low part of the token (nonce)
	 * SANITY CHECK: Be sure nonce fits into 16 bits
	 */
	low = noncectr & 0xffff;
	if ((low != noncectr++) || low == 0) {
		ERRBUF3("create_token_ref: generation number %u exceeds %u",
			noncectr - 1, 0xffff - NOFFSET);
		return TE_INTINCON;
	}

	return (TERNARY)((high << 16) | low);
}

/*
 * Check a token number and turn it into an index.
 *
 * Parameters:	TERNARY tno	ternary token from the user
 * Returned:	int       	index from the ticket
 * Errors:	TE_BADTOKEN	ternary token is invalid because:
 *				* index out of range [0 .. MAXTREES]
 *				* index is for unused slot
 *				* nonce is of old tree
 *				(te_errbuf has disambiguating string)
 *		TE_INTINCON	tree is internally inconsistent because:
 *				* nonce is 0
 *				(te_errbuf has disambiguating string)
 * EXCEPTIONS:	none
 */
static int read_token_ref(TERNARY tno)
{
	unsigned int ind;    	/* index of current tree */

	/*
	 * Get the index number and check it for validity
	 */
	ind = ((tno >> 16) & 0xffff) - IOFFSET;
	if (ind >= MAXTREES) {
		ERRBUF3("read_token_ref: index %u exceeds %d", ind, MAXTREES);
		return TE_BADTOKEN;
	}
	if (trees[ind] == NULL) {
		ERRBUF2("readbuf: token refers to unused tree index %u", ind);
		return TE_BADTOKEN;
	}

	/*
	 * We have a valid index; now validate the nonce; note we store the
	 * token in the tree, so just check that (same thing)
	 */
	if (trees[ind]->token != tno) {
		ERRBUF3("readbuf: token refers to old tree (new=%u, old=%u)",
			(unsigned int)((trees[ind]->token) & 0xffff) - IOFFSET,
			(unsigned int)(tno & 0xffff) - NOFFSET);
		return TE_BADTOKEN;
	}

	/*
	 * Check for internal consistencies
	 */
	if ((trees[ind]->token & 0xffff) == 0) {
		ERRBUF("read_token_ref: internal inconsistency: nonce = 0");
		return TE_INTINCON;
	}

	return ind;
}

/*
 * Create a new tree
 *
 * Parameters:	none
 * Returned:	TERNARY		token (if > 0); error number (if < 0)
 * Errors:	TE_BADPARAM	parameter is NULL
 *		TE_TOOMANYTS	too many trees allocated already
 *		TE_NOROOM	no memory to allocate new trees
 *				(te_errbuf has descriptive string)
 * Exceptions:	none
 */
TERNARY ternary_new(void)
{
	int cur;		/* index of current tree */
	TERNARY token;		/* new token for current tree */

	/*
	 * Check for array full
	 */
	for (cur = 0; cur < MAXTREES; cur++)
		if (trees[cur] == NULL)
			break;

	if (cur == MAXTREES) {
		ERRBUF2("ternary_new: too many trees (max %d)", MAXTREES);
		return TE_TOOMANYTS;
	}

	/*
	 * Allocate a new tree
	 */
	if ((trees[cur] = malloc(sizeof(Ttree))) == NULL) {
		ERRBUF("ternary_new: malloc: no more memory");
		return TE_NOROOM;
	}

	/*
	 * Generate token
	 */
	if (TE_ISERROR(token = create_token_ref(cur))) {
		/* error in token generation -- clean up and return */
		safefree(trees[cur]);
		trees[cur] = NULL;
		return token;
	}

	/*
	 * Now initialize tree entry
	 */
	trees[cur]->token = token;
	trees[cur]->tree_root = trees[cur]->buf = NULL;
	trees[cur]->bufn = trees[cur]->freen = 0;

	return token;
}

/*
 * Delete an exisiting tree
 *
 * Parameters:	TERNARY tno	token for the tree to be deleted
 * Returned:	uint16_t       	error code
 * Errors:	TE_BADPARAM	parameter refers to deleted, unallocated,
 *				or invalid tree (from read_token_ref()).
 *		TE_INTINCON	tree is internally inconsistent (from
 *				read_token_ref()).
 * Exceptions:	none
 */
int ternary_destroy(TERNARY tno, void (*freeptr)(void *))
{
	int cur;		/* index of current tree */
	unsigned int i, j;

	/*
	 * Check that tno refers to an existing tree;
	 * read_token_ref sets error code
	 */
	if (TE_ISERROR(cur = read_token_ref(tno)))
		return cur;

	for (i = 0; i < trees[cur]->freen; i++) {
		for (j = 0; j < BUFSIZE; j++) {
			Tnode *ptr = (trees[cur]->freearr[i] + j);
			if (ptr->splitchar == 0)
				(*freeptr)(ptr->eqkid);
			safefree(ptr);
		}
	}

	trees[cur]->token = 0;
	trees[cur]->tree_root = trees[cur]->buf = NULL;
	trees[cur]->bufn = trees[cur]->freen = 0;

	return TE_NONE;
}

/*
 * Insert the key/value into an existing tree
 *
 * Parameters:	TERNARY tno	tree to insert data into
 *		char *s		NULL terminated string (key)
 *		void *data	data to associate with key (can be NULL)
 * Returned:	int     	error code
 * Errors:	TE_BADPARAM	parameter refers to deleted, unallocated, or
 *				invalid tree (from read_token_ref()).
 *		TE_INTINCON	tree is internally inconsistent (from
 *				read_token_ref()).
 *		TE_TOOFULL	tree is full, so no new elements can be added.
 * Exceptions:	none
 */
int ternary_insert(TERNARY tno, const char *s, void *data)
{
	int cur;		/* index of current tree */
	Ttree *tree;		/* pointer to tree structure */
	int d;
	Tnode *pp, **p;

	/*
	 * Check that tno refers to an existing tree; read_token_ref sets error
	 * code.
	 */
	if (TE_ISERROR(cur = read_token_ref(tno)))
		return cur;
	
	tree = trees[cur];
	p = &(tree->tree_root);

	while ((pp = *p)) {
		if ((d = *s - pp->splitchar) == 0) {
			if (*s++ == 0) {
				return TE_EXISTS;
			}
			p = &(pp->eqkid);
		} else if (d < 0)
			p = &(pp->lokid);
		else
			p = &(pp->hikid);
	}

	for (;;) {
		if (tree->bufn-- == 0) {
			tree->buf = calloc(BUFSIZE, sizeof(Tnode));
			if (!tree->buf) {
				ERRBUF("ternary_insert: malloc: no more memory");
				return TE_NOROOM;
			}

			if (tree->freen == BUFARRAY - 1) {
				ERRBUF3("ternary_insert: freen %u equals %u",
					tree->freen,
					BUFARRAY - 1);
				return TE_TOOFULL;
			}

			tree->freearr[tree->freen++] = tree->buf;
			tree->bufn = BUFSIZE - 1;
		}
		*p = tree->buf++;

		pp = *p;
		pp->splitchar = *s;
		pp->lokid = pp->eqkid = pp->hikid = NULL;
		if (*s++ == 0) {
			pp->eqkid = (Tnode *) data;
			return TE_NONE;
		}
		p = &(pp->eqkid);
	}
}

/*
 * Return the data stored with the string
 *
 * Parameters: 	TERNARY tno	token for the tree involved
 *		char *s		the name the data is stored under
 *		void **d	the data (or NULL if you don't care about data)
 * Returned:	int		error codes
 * Errors:
 * Exceptions:
 */
int ternary_search(TERNARY tno, const char *s, void **data)
{
	int cur;
	Tnode *p;

	/*
	 * Check that tno refers to an existing tree; read_token_ref sets error
	 * code
	 */
	if (TE_ISERROR(cur = read_token_ref(tno)))
		return cur;

	p = trees[cur]->tree_root;

	while (p) {
		if (*s < p->splitchar)
			p = p->lokid;
		else if (*s == p->splitchar) {
			if (*s++ == 0) {
				if (data)
					*data = (void *)p->eqkid;
				return TE_NONE;
			}
			p = p->eqkid;
		} else
			p = p->hikid;
	}

	if (data)
		*data = (void *)NULL;
	return TE_EMPTY;
}
