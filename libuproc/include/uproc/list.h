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
#include <stdbool.h>

/** \defgroup obj_list object uproc_list
 *
 * List of homogenous items (also known as "vector" or "arraylist")
 *
 * \{
 */

/** \struct uproc_list
 * \copybrief obj_list
 *
 * See \ref obj_list for details.
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

int uproc_list_get_unsafe(const uproc_list *list, long index, void *value);

int uproc_list_get_safe(uproc_list *list, long index, void *value,
                        size_t value_size, const char *func, const char *file,
                        int line);

/* Get item at index
 *
 * Copies the data of the item at \c index into \c *value.
 * NOTE: \c value WILL BE EVALUATED TWICE.
 */
#define uproc_list_get(list, index, value)                                   \
    uproc_list_get_safe((list), (index), (value), sizeof *(value), __func__, \
                        __FILE__, __LINE__)

/** Get pointer to data
 *
 * Returns a pointer to the underlying data. Use this pointer at your own risk,
 * whenever the list is modified, this pointer could become invalid.
 */
void *uproc_list_get_ptr(uproc_list *list);

int uproc_list_set_unsafe(uproc_list *list, long index, const void *value);

int uproc_list_set_safe(uproc_list *list, long index, const void *value,
                        size_t value_size, const char *func, const char *file,
                        int line);

/** Set item at index
 *
 * Stores a copy of \c *value at position \c index, which must be less than the
 * size of the list.
 * NOTE: \c value WILL BE EVALUATED TWICE.
 */
#define uproc_list_set(list, index, value)                                   \
    uproc_list_set_safe((list), (index), (value), sizeof *(value), __func__, \
                        __FILE__, __LINE__)

int uproc_list_append_unsafe(uproc_list *list, const void *value);

int uproc_list_append_safe(uproc_list *list, const void *value,
                           size_t value_size, const char *func,
                           const char *file, int line);

/** Append item to list
 *
 * Stores a copy of \c *value at the end of the list.
 * NOTE: \c value WILL BE EVALUATED TWICE.
 */
#define uproc_list_append(list, value)                                 \
    uproc_list_append_safe((list), (value), sizeof *(value), __func__, \
                           __FILE__, __LINE__)

int uproc_list_extend_unsafe(uproc_list *list, const void *values, long n);

int uproc_list_extend_safe(uproc_list *list, const void *values, long n,
                           size_t value_size, const char *func,
                           const char *file, int line);

/** Append array of items
 *
 * Appends the \c n elements of \c values to the end of the list.
 *
 * NOTE: \c values WILL BE EVALUATED TWICE.
 */
#define uproc_list_extend(list, values, n)                                    \
    uproc_list_extend_safe((list), (values), (n), sizeof *(values), __func__, \
                           __FILE__, __LINE__)

int uproc_list_add_unsafe(uproc_list *list, const uproc_list *src);
int uproc_list_add_safe(uproc_list *list, const uproc_list *src,
                        const char *func, const char *file, int line);

/** Append all elements of another list
 *
 * Extends \c list by the items of \c src. Both lists must be created with the
 * same \c value_size argument to uproc_list_create().
 */
#define uproc_list_add(list, src) \
    uproc_list_add_safe((list), (src), __func__, __FILE__, __LINE__)

int uproc_list_pop_unsafe(uproc_list *list, void *value);
int uproc_list_pop_safe(uproc_list *list, void *value, size_t value_size,
                        const char *func, const char *file, int line);

/** Get and remove the last item
 *
 * NOTE: \c value WILL BE EVALUATED TWICE.
 *
 * */
#define uproc_list_pop(list, value)                                           \
    uproc_list_pop_safe((list), (value), sizeof *(value), __func__, __FILE__, \
                        __LINE__)

/** Returns the number of items */
long uproc_list_size(const uproc_list *list);

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
void uproc_list_map(const uproc_list *list, void (*func)(void *, void *),
                    void *opaque);

/** Apply function to all items
 *
 * Like ::uproc_list_map but without the \opaque pointer.
 */
void uproc_list_map2(const uproc_list *list, void (*func)(void *));

/** Filter list
 *
 * Removes all items from the list for which \c func(item, opaque) returns
 * false.
 *
 * \param list      list instance
 * \param func      function to call
 * \param opaque    second argument to \c function
 */
void uproc_list_filter(uproc_list *list, bool (*func)(const void *, void *),
                       void *opaque);

/** Sort list
 *
 * Sorts the list by calling \c qsort(3) on its contents.
 */
void uproc_list_sort(uproc_list *list,
                     int (*compare)(const void *, const void *));

int uproc_list_max_unsafe(const uproc_list *list,
                          int (*compare)(const void *, const void *),
                          void *value);

int uproc_list_max_safe(const uproc_list *list,
                        int (*compare)(const void *, const void *), void *value,
                        size_t value_size, const char *func, const char *file,
                        int line);

/* Get greatest item according to \c compare
 *
 * The semantics of \c compare correspond to those expected by \c qsort(3).
 *
 * NOTE: \c value WILL BE EVALUATED TWICE.
 */
#define uproc_list_max(list, compare, value)                                   \
    uproc_list_max_safe((list), (compare), (value), sizeof *(value), __func__, \
                        __FILE__, __LINE__)

/** \} obj_list */

/**
 * \} grp_datastructs_list
 * \} grp_datastructs
 */
#endif
