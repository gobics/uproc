/* Generic list container
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "uproc/error.h"
#include "uproc/list.h"

#define MIN_CAPACITY 32
#define MAX_SIZE (LONG_MAX)


struct uproc_list_s
{
    /* Number of stored values */
    long size;

    /* Allocated size in number of _values_ */
    long capacity;

    /* Number of bytes one value requires */
    size_t value_size;

    /* Stored data */
    unsigned char *data;
};


/* Reallocate the list data pointer to the given new capacity or at
 * least MIN_CAPACITY. Fails if memory can't be reallocated */
static int
list_realloc(struct uproc_list_s *list, long n)
{
    unsigned char *tmp;
    if (n < MIN_CAPACITY) {
        n = MIN_CAPACITY;
    }
    tmp = realloc(list->data, n * list->value_size);
    if (!tmp) {
        return uproc_error(UPROC_ENOMEM);
    }
    list->data = tmp;
    list->capacity = n;
    return 0;
}


/* Doubles the capacity of a list. If (value_size * capacity) would exceed
 * MAX_SIZE, use (MAX_SIZE / value_size) instead of doubling.
 * Fails if the size is already (MAX_SIZE / value_size).  */
static int
list_grow(struct uproc_list_s *list, long n)
{
    long cap_req, cap_new, cap_max;

    cap_req = list->size + n;
    if (cap_req < list->capacity) {
        return 0;
    }

    cap_max = MAX_SIZE / list->value_size;
    if (list->capacity == cap_max || cap_req > cap_max) {
        return uproc_error_msg(UPROC_EINVAL, "list too big");
    }

    if (cap_max / 2 > list->capacity) {
        cap_new = list->capacity * 2;
    }
    else {
        cap_new = cap_max;
    }
    if (cap_new < cap_req) {
        cap_new = cap_req;
    }
    return list_realloc(list, cap_new);
}


uproc_list *
uproc_list_create(size_t value_size)
{
   struct uproc_list_s *list;
   if (!value_size) {
       uproc_error_msg(UPROC_EINVAL, "value size must be non-zero");
       return NULL;
   }

   list = malloc(sizeof *list);
   if (!list) {
       uproc_error(UPROC_ENOMEM);
       return NULL;
   }
   list->size = list->capacity = 0;
   list->value_size = value_size;
   list->data = NULL;

   if (list_realloc(list, 0)) {
       free(list);
       return NULL;
   }
   return list;
}


void
uproc_list_destroy(uproc_list *list)
{
    if (!list) {
        return;
    }
    free(list->data);
    free(list);
}


void
uproc_list_clear(uproc_list *list)
{
    list->size = 0;
}


int
uproc_list_get(const uproc_list *list, long index, void *value)
{
    long idx = index;
    if (idx < 0) {
        idx += list->size;
    }
    if (idx < 0 || idx >= list->size) {
        return uproc_error_msg(
            UPROC_EINVAL, "list index %ld out of range", index);
    }
    memcpy(value, list->data + idx * list->value_size, list->value_size);
    return 0;
}


size_t
uproc_list_get_all(const uproc_list *list, void *buf, size_t sz)
{
    size_t needed = list->value_size * list->size;
    if (needed < sz) {
        sz = needed;
    }
    memcpy(buf, list->data, sz);
    return needed;
}


int
uproc_list_set(uproc_list *list, long index, const void *value)
{
    long idx = index;
    if (idx < 0) {
        idx += list->size;
    }
    if (idx < 0 || idx >= list->size) {
        return uproc_error_msg(
            UPROC_EINVAL, "list index %ld out of range", index);
    }
    memcpy(list->data + idx * list->value_size, value, list->value_size);
    return 0;
}


int
uproc_list_append(uproc_list *list, const void *value)
{
    int res = list_grow(list, 1);
    if (res) {
        return res;
    }
    list->size++;
    return uproc_list_set(list, list->size - 1, value);
}


int
uproc_list_extend(uproc_list *list, const void *values, long n)
{
    int res;
    if (n < 0) {
        return uproc_error_msg(UPROC_EINVAL, "negative number of items");
    }
    if (n > (LONG_MAX - list->size)) {
        return uproc_error_msg(UPROC_EINVAL, "list too big");
    }

    res = list_grow(list, n);
    if (res) {
        return res;
    }
    memcpy(list->data + list->size * list->value_size, values, n * list->value_size);
    list->size += n;
    return 0;
}


int
uproc_list_add(uproc_list *list, const uproc_list *src)
{
    if (list->value_size != src->value_size) {
        return uproc_error_msg(UPROC_EINVAL,
                               "can't add lists with different value sizes");
    }
    return uproc_list_extend(list, src->data, src->size);
}


int
uproc_list_pop(uproc_list *list, void *value)
{
    if (list->size < 1) {
        return uproc_error_msg(UPROC_EINVAL, "pop from empty list");
    }
    uproc_list_get(list, list->size - 1, value);
    list->size--;
    if (list->size > list->capacity / 4) {
        return 0;
    }
    return list_realloc(list, list->capacity / 2);
}


long
uproc_list_size(const uproc_list *list)
{
    return list->size;
}


void
uproc_list_map(const uproc_list *list, void(*func)(void *, void *),
               void *opaque)
{
    for (long i = 0; i < list->size; i++) {
        func(list->data + i * list->value_size, opaque);
    }
}
