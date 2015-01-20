/* Copyright 2015 Peter Meinicke, Robin Martinjak, Manuel Landesfeind
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

/** \file uproc/dict.h
 *
 * Module: \ref grp_datastructs_dict
 *
 * \weakgroup grp_datastructs
 * \{
 * \weakgroup grp_datastructs_dict
 *
 * \details
 * Generic dictionary container.
 *
 * \{
 */

#ifndef UPROC_DICT_H
#define UPROC_DICT_H

#include "uproc/io.h"

/** \defgroup obj_dict object uproc_dict
 *
 * Dictionary container (also known as "hash table" or "hash map").
 *
 * \note Keys are hashed and compared bytewise, be careful about padding when
 * you use structs as keys, e.g. use \code memset(&s, 0, s) \endcode to make
 * sure all padding bytes are zero.
 *
 * \{
 */

/** \struct uproc_dict
 * \copybrief obj_dict
 *
 * See \ref obj_dict for details.
 */
typedef struct uproc_dict_s uproc_dict;

/** Dict specific return codes */
enum {
    UPROC_DICT_KEY_NOT_FOUND = -404,
};

enum {
    /** Maximum number of bytes that can be used for keys */
    UPROC_DICT_KEY_SIZE_MAX = 32,

    /** Maximum number of bytes that can be used for values */
    UPROC_DICT_VALUE_SIZE_MAX = 32,

    UPROC_DICT_STORE_SIZE_MAX = 4096,
};

/** Create an empty dict
 *
 * If \c key_size or \c value_size is 0, the dict will treat keys (resp.
 * values) as strings, with a maximum allowed length of
 * ::UPROC_DICT_KEY_SIZE_MAX - 1 (resp. ::UPROC_DICT_VALUE_SIZE_MAX - 1)
 *
 * \param key_size          size of the stored keys
 * \param value_size        size of the stored values
 */
uproc_dict *uproc_dict_create(size_t key_size, size_t value_size);

/** Destroy dict object */
void uproc_dict_destroy(uproc_dict *dict);

/** Remove all items */
int uproc_dict_clear(uproc_dict *dict);

int uproc_dict_get_safe(const uproc_dict *dict, const void *key,
                        size_t key_size, void *value, size_t value_size);

/* Get item with key
 *
 * Copies the data of the item corresponding to key \c key into \c *value.
 * NOTE: \c key AND \c value WILL BE EVALUATED TWICE.
 */
#define uproc_dict_get(dict, key, value) \
    uproc_dict_get_safe((dict), (key), sizeof *(key), (value), sizeof *(value))

int uproc_dict_set_safe(uproc_dict *dict, const void *key, size_t key_size,
                        const void *value, size_t value_size);

/** Set value for key
 *
 * Stores a copy of \c *value at position \c index, which must be less than the
 * size of the dict.
 * NOTE: \c key AND \c value WILL BE EVALUATED TWICE.
 */
#define uproc_dict_set(dict, key, value) \
    uproc_dict_set_safe((dict), (key), sizeof *(key), (value), sizeof *(value))

int uproc_dict_remove_safe(uproc_dict *dict, const void *key, size_t key_size);

/** Remove key
 *
 * NOTE: \c key WILL BE EVALUATED TWICE.
 */
#define uproc_dict_remove(dict, key) \
    uproc_dict_remove_safe((dict), (key), sizeof *(key))

int uproc_dict_pop_safe(uproc_dict *dict, void *key, size_t key_size,
                        void *value, size_t value_size);

/** Get and remove an item
 *
 * NOTE: \c key AND \c value WILL BE EVALUATED TWICE.
 *
 * */
#define uproc_dict_pop(dict, key, value) \
    uproc_dict_pop_safe((dict), (key), sizeof *(key), (value), sizeof *(value))

/** Return the number of items */
long uproc_dict_size(const uproc_dict *dict);

/** Load dict from stream
 *
 * See ::uproc_dict_load for details.
 */
uproc_dict *uproc_dict_loads(size_t key_size, size_t value_size,
                             int (*scan)(const char *, void *, void *,
                                         void *),
                             void *opaque, uproc_io_stream *stream);

/** Load dict from file
 *
 * \c scan should convert the string pointed to by the first argument to key
 * and value (second and third parameters). The fourth parameter to \c scan is
 * the user supplied \c opaque pointer.
 *
 * \param key_size      size of the stored keys (see ::uproc_dict_create)
 * \param value_size    size of the stored values (see ::uproc_dict_create)
 * \param scan          pointer to function converting string to key and value
 * \param opaque        fourth argument to \c scan
 * \param pathfmt       printf format string for file path
 * \param ...           format string arguments
 */
uproc_dict *uproc_dict_load(size_t key_size, size_t value_size,
                            int (*scan)(const char *, void *, void *, void *),
                            void *opaque, enum uproc_io_type iotype,
                            const char *pathfmt, ...);
/** Load dict from file
 *
 * Like ::uproc_dict_load, but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_dict *uproc_dict_loadv(size_t key_size, size_t value_size,
                             int (*scan)(const char *, void *, void *,
                                         void *),
                             void *opaque, enum uproc_io_type iotype,
                             const char *pathfmt, va_list ap);

int uproc_dict_stores(const uproc_dict *dict,
                      int (*format)(char *, const void *, const void *,
                                    void *),
                      void *opaque, uproc_io_stream *stream);

int uproc_dict_storev(const uproc_dict *dict,
                      int (*format)(char *, const void *, const void *,
                                    void *),
                      void *opaque, enum uproc_io_type iotype,
                      const char *pathfmt, va_list ap);

int uproc_dict_store(const uproc_dict *dict,
                     int (*format)(char *, const void *, const void *,
                                   void *),
                     void *opaque, enum uproc_io_type iotype,
                     const char *pathfmt, ...);

/** \} */

/** \defgroup obj_dictiter object uproc_dictiter
 *
 * Dict iterator (unordered).
 * \{
 */
typedef struct uproc_dictiter_s uproc_dictiter;

/** Create unordered iterator
 *
 * If the dict being iterated is modified, correct behaviour of the iterator is
 * not guaranteed (i.o.w. don't do it).
 *
 * \param dict     dict instance
 *
 * \return
 * Returns a new dict iterator instance.
 * If memory allocation fails, returns NULL and sets ::uproc_errno to
 * ::UPROC_ENOMEM.
 */
uproc_dictiter *uproc_dictiter_create(const uproc_dict *dict);

int uproc_dictiter_next_safe(uproc_dictiter *iter, void *key, size_t key_size,
                             void *value, size_t value_size);
/** Obtain next key/value pair
 *
 * NOTE: \c key AND \c value WILL BE EVALUATED TWICE.
 *
 *  \param iter     dict iterator
 *  \param key      _OUT_: value of next key
 *  \param value    _OUT_: value of next item
 *
 *  \return
 *  Returns 0 if an item was produced, -1 in case of an error and 1 if the
 *  iterator is exhausted. If the returned value is not 0, \c *key and \c
 *  *value are not modified.
 */
#define uproc_dictiter_next(iter, key, value)                       \
    uproc_dictiter_next_safe((iter), (key), sizeof *(key), (value), \
                             sizeof *(value))

/** Destroy dict iterator */
void uproc_dictiter_destroy(uproc_dictiter *iter);

/** \} */

/**
 * \}
 * \}
 */
#endif
