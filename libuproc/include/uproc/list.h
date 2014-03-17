/* Copyright 2014 Peter Meinicke, Robin Martinjak
 *
 * This file is part of libuproc.
 *
 * libuproc is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libuproc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libuproc.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file uproc/list.h
 *
 * Module: \ref grp_datastructs_list
 *
 * \weakgroup grp_datastructs
 * \{
 * \weakgroup grp_datastructs_list
 *
 * \details
 * Generic list container implemented as a growing array.
 *
 * \{
 */

#ifndef UPROC_LIST_H
#define UPROC_LIST_H

#include <stdlib.h>
#include <string.h>


/** \defgroup obj_list object uproc_list
 * \{
 */
typedef struct uproc_list_s uproc_list;


/** Create an empty list
 *
 * \param value_size        size of the stored values
 */
uproc_list *uproc_list_create(size_t value_size);


/** Destroy list object */
void uproc_list_destroy(uproc_list *list);


/** Remove all items */
void uproc_list_clear(uproc_list *list);


/* Get item at index
 *
 * Copies the data of the item at \c index into \c *value.
 */
int uproc_list_get(const uproc_list *list, size_t index, void *value);


/** Get all items
 *
 * Copies the whole list data, but at most \c sz bytes, into the array pointed
 * to by \c dest.
 *
 * \param list  list to copy from
 * \param buf   buffer to copy into
 * \param sz    size of \c dest in bytes
 *
 * \retval  The number of copied bytes, or how many bytes would have been
 * copied, if this number exceeds \c sz.
 */
size_t uproc_list_get_all(const uproc_list *list, void *buf, size_t sz);


/** Set item at index
 *
 * Stores a copy of \c *value at position \c index, which must be less than the
 * size of the list.
 */
int uproc_list_set(uproc_list *list, size_t index, const void *value);


/** Append item to list
 *
 * Stores a copy of \c *value at the end of the list.
 */
int uproc_list_append(uproc_list *list, const void *value);


/** Returns the number of items */
size_t uproc_list_size(const uproc_list *list);


/** Apply function to all items
 *
 * The first argument passed to \c func is a pointer to one item of the
 * list. This is not a copy, so modifying it will also affect the stored value.
 * The second argument to \c func is the user-supplied \c opaque pointer.
 *
 * \param list      list instance
 * \param func      function to call
 * \param opaque    second argument to \c function
 */
void uproc_list_map(const uproc_list *list, void(*func)(void *, void *),
                    void *opaque);

/** \} */


/**
 * \}
 * \}
 */
#endif
