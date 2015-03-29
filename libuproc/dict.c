/* Generic dictionary (or map) container
 *
 * Copyright 2015 Peter Meinicke, Robin Martinjak, Manuel Landesfeind
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

#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "uproc/error.h"
#include "uproc/dict.h"
#include "uproc/list.h"

static long bucket_count_values[] = {1823,   3607,   8191,   15859,
                                     28349,  57859,  119563, 241037,
                                     450887, 927587, 2041859};
#define BUCKET_COUNT_VALUES_SIZE \
    (sizeof bucket_count_values / sizeof bucket_count_values[0])

typedef unsigned char
    key_value_buffer[UPROC_DICT_KEY_SIZE_MAX + UPROC_DICT_VALUE_SIZE_MAX];

struct uproc_dict_s
{
    /* Number of stored elements */
    long size;

    /* Element of bucket_count_values representing the number of buckets */
    int bucket_count_index;

    /* Number of bytes one key requires */
    size_t key_size;

    /* Number of bytes one value requires */
    size_t value_size;

    /* Lists storing elements of type unsigned char[key_size + value_size] */
    uproc_list **buckets;
};

struct uproc_dictiter_s
{
    const struct uproc_dict_s *dict;
    long bucket_index;
    long bucket_item_index;
};

static int iter_next(uproc_dictiter *iter, void *key, void *value);

static inline long bucket_count(const uproc_dict *dict)
{
    if (!dict->buckets) {
        return 0;
    }
    return bucket_count_values[dict->bucket_count_index];
}

static inline size_t bucket_item_size(const uproc_dict *dict)
{
    return (dict->key_size ? dict->key_size : UPROC_DICT_KEY_SIZE_MAX) +
           (dict->value_size ? dict->value_size : UPROC_DICT_VALUE_SIZE_MAX);
}

static inline void copy_mem_or_string(void *dest, const void *src, size_t size,
                                      size_t max_size)
{
    if (size) {
        memcpy(dest, src, size);
    } else {
        strncpy(dest, src, max_size);
    }
}

static inline void buf_to_kv(const uproc_dict *dict, void *key, void *value,
                             const key_value_buffer buf)
{
    if (key) {
        copy_mem_or_string(key, buf, dict->key_size, UPROC_DICT_KEY_SIZE_MAX);
    }
    copy_mem_or_string(value, buf + (dict->key_size ? dict->key_size
                                                    : UPROC_DICT_KEY_SIZE_MAX),
                       dict->value_size, UPROC_DICT_VALUE_SIZE_MAX);
}

static int compare_bucket_items(const void *item1, const void *item2,
                                size_t key_size)
{
    if (key_size) {
        return memcmp(item1, item2, key_size);
    }
    return strncmp(item1, item2, UPROC_DICT_KEY_SIZE_MAX);
}

static long find_in_bucket(const uproc_dict *dict, uproc_list *bucket,
                           const void *key)
{
    key_value_buffer buf;
    long res = -1;
    for (long i = 0; i < uproc_list_size(bucket); i++) {
        (void)uproc_list_get_safe(bucket, i, buf, bucket_item_size(dict));
        if (!compare_bucket_items(key, buf, dict->key_size)) {
            res = i;
            break;
        }
    }
    return res;
}

static long hash(const unsigned char *key, size_t key_size)
{
    size_t len = key_size ? key_size : strlen((char *)key);
    unsigned long hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + key[i];
    }
    return hash % LONG_MAX;
}

static int check_sizes(const uproc_dict *dict, const void *key, size_t key_size,
                       const void *value, size_t value_size)
{
    if ((dict->key_size && key_size != dict->key_size) ||
        (!dict->key_size && !key && key_size != UPROC_DICT_KEY_SIZE_MAX)) {
        return uproc_error_msg(
            UPROC_EINVAL,
            "dict key size %lu (possibly) doesn't match "
            "size of passed object %lu",
            (unsigned long)(dict->key_size ? dict->key_size
                                           : UPROC_DICT_KEY_SIZE_MAX),
            (unsigned long)key_size);
    }
    if (!dict->key_size && key && strlen(key) >= UPROC_DICT_KEY_SIZE_MAX) {
        return uproc_error_msg(UPROC_EINVAL, "dict key string too long");
    }

    if ((dict->value_size && value_size != dict->value_size) ||
        (!dict->value_size && !value &&
         value_size != UPROC_DICT_VALUE_SIZE_MAX)) {
        return uproc_error_msg(
            UPROC_EINVAL,
            "dict value size %lu (possibly) doesn't match "
            "size of passed object %lu",
            (unsigned long)(dict->value_size ? dict->value_size
                                             : UPROC_DICT_VALUE_SIZE_MAX),
            (unsigned long)value_size);
    }
    if (!dict->value_size && value &&
        strlen(value) >= UPROC_DICT_VALUE_SIZE_MAX) {
        return uproc_error_msg(UPROC_EINVAL, "dict value string too long");
    }
    return 0;
}

static void free_buckets(uproc_list **buckets, long bucket_count)
{
    for (long i = 0; i < bucket_count; i++) {
        uproc_list_destroy(buckets[i]);
    }
    free(buckets);
}

/** Allocates bucket_count buckets suitable for dict */
static uproc_list **alloc_buckets(uproc_dict *dict, long bucket_count)
{
    uproc_list **buckets = malloc(bucket_count * sizeof *buckets);
    if (!buckets) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    for (long i = 0; i < bucket_count; i++) {
        buckets[i] = uproc_list_create(bucket_item_size(dict));
        if (!buckets[i]) {
            free_buckets(buckets, i);
            return NULL;
        }
    }
    return buckets;
}

static int resize(uproc_dict *dict, bool grow)
{
    int new_bucket_count_index;
    if (grow) {
        if (dict->bucket_count_index == BUCKET_COUNT_VALUES_SIZE - 1) {
            return 0;
        }
        new_bucket_count_index = dict->bucket_count_index + 1;
    } else {
        if (dict->bucket_count_index == 0) {
            return 0;
        }
        new_bucket_count_index = dict->bucket_count_index - 1;
    }

    long new_bucket_count = bucket_count_values[new_bucket_count_index];
    uproc_list **new_buckets = alloc_buckets(dict, new_bucket_count);
    if (!new_buckets) {
        return -1;
    }

    key_value_buffer buf;
    for (long i = 0; i < bucket_count(dict); i++) {
        for (long k = 0; k < uproc_list_size(dict->buckets[i]); k++) {
            int res;
            res = uproc_list_get_safe(dict->buckets[i], k, buf,
                                      bucket_item_size(dict));
            // This should never fail.
            uproc_assert(!res);

            unsigned long new_bucket_index =
                hash(buf, dict->key_size) % new_bucket_count;
            res = uproc_list_append_safe(new_buckets[new_bucket_index], buf,
                                         bucket_item_size(dict));
            if (res) {
                free_buckets(new_buckets, new_bucket_count);
                return -1;
            }
        }
    }

    free_buckets(dict->buckets, bucket_count(dict));
    dict->buckets = new_buckets;
    dict->bucket_count_index = new_bucket_count_index;
    return 0;
}

uproc_dict *uproc_dict_create(size_t key_size, size_t value_size)
{
    uproc_assert(key_size <= UPROC_DICT_KEY_SIZE_MAX);
    uproc_assert(value_size <= UPROC_DICT_VALUE_SIZE_MAX);

    uproc_dict *dict = malloc(sizeof *dict);
    if (!dict) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    *dict = (struct uproc_dict_s){0};

    dict->key_size = key_size;
    dict->value_size = value_size;
    dict->size = 0;

    if (uproc_dict_clear(dict)) {
        free(dict);
        return NULL;
    }
    return dict;
}

int uproc_dict_clear(uproc_dict *dict)
{
    uproc_list **new_buckets = alloc_buckets(dict, bucket_count_values[0]);
    if (!new_buckets) {
        return -1;
    }
    free_buckets(dict->buckets, bucket_count(dict));
    dict->size = 0;
    dict->bucket_count_index = 0;
    dict->buckets = new_buckets;
    return 0;
}

void uproc_dict_destroy(uproc_dict *dict)
{
    if (!dict) {
        return;
    }
    free_buckets(dict->buckets, bucket_count(dict));
    free(dict);
}

static int dict_set(uproc_dict *dict, const void *key, const void *value)
{
    int res = 0;
    unsigned long bucket_index = hash(key, dict->key_size) % bucket_count(dict);
    uproc_list *bucket = dict->buckets[bucket_index];

    // Copy key and value into the buffer that will be copied into the bucket.
    key_value_buffer buf;
    copy_mem_or_string(buf, key, dict->key_size, UPROC_DICT_KEY_SIZE_MAX);
    copy_mem_or_string(
        buf + (dict->key_size ? dict->key_size : UPROC_DICT_KEY_SIZE_MAX),
        value, dict->value_size, UPROC_DICT_VALUE_SIZE_MAX);

    long index = find_in_bucket(dict, bucket, key);
    if (index < 0) {
        res = uproc_list_append_safe(bucket, buf, bucket_item_size(dict));
        if (res) {
            return -1;
        }
        uproc_assert(dict->size < LONG_MAX);
        dict->size += 1;

        // Resize if necessary
        if (dict->size > bucket_count(dict)) {
            res = resize(dict, true);
            if (res) {
                return -1;
            }
        }
    } else {
        res = uproc_list_set_safe(bucket, index, buf, bucket_item_size(dict));
        uproc_assert(!res);
    }
    return 0;
}

int uproc_dict_set_safe(uproc_dict *dict, const void *key, size_t key_size,
                        const void *value, size_t value_size)
{
    if (check_sizes(dict, key, key_size, value, value_size)) {
        return -1;
    }
    return dict_set(dict, key, value);
}

static int dict_get(const uproc_dict *dict, const void *key, void *value)
{
    unsigned long bucket_index = hash(key, dict->key_size) % bucket_count(dict);
    uproc_list *bucket = dict->buckets[bucket_index];
    long index = find_in_bucket(dict, bucket, key);
    if (index < 0) {
        return UPROC_DICT_KEY_NOT_FOUND;
    }
    key_value_buffer buf;
    (void)uproc_list_get_safe(bucket, index, buf, bucket_item_size(dict));
    buf_to_kv(dict, NULL, value, buf);
    return 0;
}

int uproc_dict_get_safe(const uproc_dict *dict, const void *key,
                        size_t key_size, void *value, size_t value_size)
{
    if (check_sizes(dict, key, key_size, NULL, value_size)) {
        return -1;
    }
    return dict_get(dict, key, value);
}

static int dict_remove(uproc_dict *dict, const void *key)
{
    unsigned long bucket_index = hash(key, dict->key_size) % bucket_count(dict);
    uproc_list *bucket = dict->buckets[bucket_index];
    long index = find_in_bucket(dict, bucket, key);
    if (index < 0) {
        return UPROC_DICT_KEY_NOT_FOUND;
    }
    key_value_buffer buf;
    (void)uproc_list_get_safe(bucket, index, buf, bucket_item_size(dict));
    // Delete the element at `index` by swapping it with the last one.
    // This changes the order, but that's not an issue.
    (void)uproc_list_pop_safe(bucket, buf, bucket_item_size(dict));
    if (index == uproc_list_size(bucket)) {
        // The element to be removed was at the end of the list, nothing to do.
    } else {
        int res =
            uproc_list_set_safe(bucket, index, buf, bucket_item_size(dict));
        uproc_assert(!res);
    }
    uproc_assert(dict->size > 0);
    dict->size -= 1;

    if (dict->size * 4 < bucket_count(dict)) {
        return resize(dict, false);
    }
    return 0;
}

int uproc_dict_remove_safe(uproc_dict *dict, const void *key, size_t key_size)
{
    if (check_sizes(dict, key, key_size, NULL, dict->value_size)) {
        return -1;
    }
    return dict_remove(dict, key);
}

long uproc_dict_size(const uproc_dict *dict)
{
    return dict->size;
}

uproc_dict *uproc_dict_loads(size_t key_size, size_t value_size,
                             int (*scan)(const char *, void *, void *, void *),
                             void *opaque, uproc_io_stream *stream)
{
    int res;
    uproc_dict *dict = uproc_dict_create(key_size, value_size);
    if (!dict) {
        return NULL;
    }

    char *line = NULL;
    size_t line_sz = 0;

    res = uproc_io_getline(&line, &line_sz, stream);
    if (res == -1) {
        return NULL;
    }

    long size;
    res = sscanf(line, "[%ld]", &size);
    if (res != 1) {
        uproc_error_msg(UPROC_EINVAL, "invalid dict header");
        return NULL;
    }

    while (size--) {
        res = uproc_io_getline(&line, &line_sz, stream);
        if (res == -1) {
            break;
        }

        unsigned char key[UPROC_DICT_KEY_SIZE_MAX],
            value[UPROC_DICT_VALUE_SIZE_MAX];
        char *p = strchr(line, '\n');
        if (p) {
            *p = '\0';
        }
        res = scan(line, key, value, opaque);
        if (res) {
            break;
        }
        res = uproc_dict_set_safe(dict, key, key_size, value, value_size);
        if (res) {
            break;
        }
    }
    if (res) {
        uproc_dict_destroy(dict);
        dict = NULL;
    }
    free(line);
    return dict;
}

uproc_dict *uproc_dict_loadv(size_t key_size, size_t value_size,
                             int (*scan)(const char *, void *, void *, void *),
                             void *opaque, enum uproc_io_type iotype,
                             const char *pathfmt, va_list ap)
{
    uproc_io_stream *stream = uproc_io_openv("r", iotype, pathfmt, ap);
    if (!stream) {
        return NULL;
    }
    uproc_dict *dict =
        uproc_dict_loads(key_size, value_size, scan, opaque, stream);
    uproc_io_close(stream);
    return dict;
}

uproc_dict *uproc_dict_load(size_t key_size, size_t value_size,
                            int (*scan)(const char *, void *, void *, void *),
                            void *opaque, enum uproc_io_type iotype,
                            const char *pathfmt, ...)
{
    va_list ap;
    va_start(ap, pathfmt);
    uproc_dict *dict = uproc_dict_loadv(key_size, value_size, scan, opaque,
                                        iotype, pathfmt, ap);
    va_end(ap);
    return dict;
}

int uproc_dict_stores(const uproc_dict *dict,
                      int (*format)(char *, const void *, const void *, void *),
                      void *opaque, uproc_io_stream *stream)
{
    int res = 0;
    uproc_dictiter *iter = uproc_dictiter_create(dict);
    if (!iter) {
        return -1;
    }
    unsigned char key[UPROC_DICT_KEY_SIZE_MAX],
        value[UPROC_DICT_VALUE_SIZE_MAX];
    char buf[UPROC_DICT_STORE_BUFFER_SIZE];

    res = uproc_io_printf(stream, "[%ld]\n", uproc_dict_size(dict));
    if (res < 0) {
        return -1;
    }

    while (res = iter_next(iter, key, value), !res) {
        res = format(buf, key, value, opaque);
        if (res) {
            res = -1;
            break;
        }
        res = uproc_io_puts(buf, stream);
        if (res < 0) {
            break;
        }
    }
    if (res == 1) {
        res = 0;
    }
    uproc_dictiter_destroy(iter);
    return res;
}

int uproc_dict_storev(const uproc_dict *dict,
                      int (*format)(char *, const void *, const void *, void *),
                      void *opaque, enum uproc_io_type iotype,
                      const char *pathfmt, va_list ap)
{
    int res;
    uproc_io_stream *stream = uproc_io_openv("w", iotype, pathfmt, ap);
    if (!stream) {
        return -1;
    }
    res = uproc_dict_stores(dict, format, opaque, stream);
    uproc_io_close(stream);
    return res;
}

int uproc_dict_store(const uproc_dict *dict,
                     int (*format)(char *, const void *, const void *, void *),
                     void *opaque, enum uproc_io_type iotype,
                     const char *pathfmt, ...)
{
    va_list ap;
    va_start(ap, pathfmt);
    int res = uproc_dict_storev(dict, format, opaque, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

void uproc_dict_map(const uproc_dict *dict,
                    void (*func)(void *, void *, void *), void *opaque)
{
    struct uproc_dictiter_s iter = {dict, 0, 0};
    int res;
    key_value_buffer buf;
    void *key = buf, *value = buf + UPROC_DICT_KEY_SIZE_MAX;
    while (res = iter_next(&iter, key, value), !res) {
        func(key, value, opaque);
    }
}

uproc_dictiter *uproc_dictiter_create(const uproc_dict *dict)
{
    uproc_dictiter *iter = malloc(sizeof *iter);
    if (!iter) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    iter->dict = dict;
    iter->bucket_index = 0;
    iter->bucket_item_index = 0;
    return iter;
}

static int iter_next(uproc_dictiter *iter, void *key, void *value)
{
    if (iter->bucket_index == bucket_count(iter->dict)) {
        return 1;
    }
    const uproc_list *bucket = iter->dict->buckets[iter->bucket_index];
    if (iter->bucket_item_index == uproc_list_size(bucket)) {
        iter->bucket_index++;
        iter->bucket_item_index = 0;
        return iter_next(iter, key, value);
    }
    key_value_buffer buf;
    uproc_list_get_safe(bucket, iter->bucket_item_index, buf,
                        bucket_item_size(iter->dict));
    buf_to_kv(iter->dict, key, value, buf);
    iter->bucket_item_index++;
    return 0;
}

int uproc_dictiter_next_safe(uproc_dictiter *iter, void *key, size_t key_size,
                             void *value, size_t value_size)
{
    if (check_sizes(iter->dict, NULL, key_size, NULL, value_size)) {
        return -1;
    }
    return iter_next(iter, key, value);
}

void uproc_dictiter_destroy(uproc_dictiter *iter)
{
    free(iter);
}
