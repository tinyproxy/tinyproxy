/* $Id: vector.h,v 1.2 2003-05-29 20:47:51 rjkaes Exp $
 *
 * A vector implementation.  The vector can be of an arbritrary length, and
 * the data for each entry is an lump of data (the size is stored in the
 * vector.)
 *
 * Copyright (C) 2002  Robert James Kaes (rjkaes@flarenet.com)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _VECTOR_H
#define _VECTOR_H

/* Allow the use in C++ code. */
#if defined(__cplusplus)
extern "C" {
#endif

/*
 * We're using a typedef here to "hide" the implementation details of the
 * vector.  Sure, it's a pointer, but the struct is hidden in the C file.
 * So, just use the vector_t like it's a cookie. :)
 */
typedef struct vector_s *vector_t;

/*
 * vector_create() takes no arguments.
 * vector_delete() is self explanatory.
 */
extern vector_t vector_create(void);
extern int vector_delete(vector_t vector);

/*
 * Append an entry to the end of the vector.  When you insert a piece of
 * data into the vector, the data will be duplicated, so you must free your
 * copy if it was created on the heap.  The data must be non-NULL and the
 * length must be greater than zero.
 *
 * Returns: negative on error
 *          0 upon successful insert.
 */
extern int vector_append(vector_t vector, void *data, ssize_t len);

/*
 * A pointer to the data at position "pos" (zero based) is returned in the
 * "data" pointer.  If the vector is out of bound, data is set to NULL.
 *
 * The pointer points to the actual data in the vector, so you have
 * the power to modify the data, but do it responsibly since the
 * library doesn't take any steps to prevent you from messing up the
 * vector.  (A better rule is, don't modify the data since you'll
 * likely mess up the "length" parameter of the data.)  However, DON'T
 * try to realloc or free the data; doing so will break the vector.
 *
 * Returns: negative upon an error
 *          length of data if position is valid
 */
extern ssize_t vector_getentry(vector_t vector, size_t pos, void **data);

/*
 * Returns the number of enteries (or the length) of the vector.
 *
 * Returns: negative if vector is not valid
 *          positive length of vector otherwise
 */
extern ssize_t vector_length(vector_t vector);

#if defined(_cplusplus)
}
#endif /* C++ */

#endif /* _VECTOR_H */
