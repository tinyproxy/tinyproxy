/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 2002 Robert James Kaes <rjkaes@users.sourceforge.net>
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

/* See 'vector.c' for detailed information. */

#ifndef _VECTOR_H
#define _VECTOR_H

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
extern vector_t vector_create (void);
extern int vector_delete (vector_t vector);

/*
 * When you insert a piece of data into the vector, the data will be
 * duplicated, so you must free your copy if it was created on the heap.
 * The data must be non-NULL and the length must be greater than zero.
 *
 * Returns: negative on error
 *          0 upon successful insert.
 */
extern int vector_append (vector_t vector, void *data, size_t len);
extern int vector_prepend (vector_t vector, void *data, size_t len);

/*
 * A pointer to the data at position "pos" (zero based) is returned and the
 * size pointer contains the length of the data stored.
 *
 * The pointer points to the actual data in the vector, so you have
 * the power to modify the data, but do it responsibly since the
 * library doesn't take any steps to prevent you from messing up the
 * vector.  (A better rule is, don't modify the data since you'll
 * likely mess up the "length" parameter of the data.)  However, DON'T
 * try to realloc or free the data; doing so will break the vector.
 *
 * If "size" is NULL the size of the data is not returned.
 *
 * Returns: NULL on error
 *          valid pointer to data
 */
extern void *vector_getentry (vector_t vector, size_t pos, size_t * size);

/*
 * Returns the number of enteries (or the length) of the vector.
 *
 * Returns: negative if vector is not valid
 *          positive length of vector otherwise
 */
extern ssize_t vector_length (vector_t vector);

#endif /* _VECTOR_H */
