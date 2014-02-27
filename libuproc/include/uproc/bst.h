/** \file uproc/bst.h
 *
 * Binary search tree
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

#ifndef UPROC_BST_H
#define UPROC_BST_H

#include <stdlib.h>
#include <stdint.h>
#include "uproc/common.h"
#include "uproc/word.h"

/** Key type */
union uproc_bst_key
{
    /** uintmax_t */
    uintmax_t uint;

    /** struct uproc_word */
    struct uproc_word word;
};

/** Which member of union uproc_bst_key is used */
enum uproc_bst_keytype
{
    /** uintmax_t */
    UPROC_BST_UINT,

    /** struct uproc_word */
    UPROC_BST_WORD,
};

/** BST specific return codes */
enum
{
    /** BST doesn't contain an item with the given key */
    UPROC_BST_KEY_NOT_FOUND = -404,

    /** BST already contains an item with the given key */
    UPROC_BST_KEY_EXISTS = -403,
};

/** Opaque type for binary search trees */
typedef struct uproc_bst_s uproc_bst;

/** BST iterator */
typedef struct uproc_bstiter_s uproc_bstiter;

/** Callback function type for uproc_bst_walk */
typedef int (*uproc_bst_cb_walk)(union uproc_bst_key, const void*, void*);

/** Callback function type for uproc_bst_remove and uproc_bst_clear */
typedef int (*uproc_bst_cb_remove)(const void*);

/** Initialize an empty binary search tree
 *
 * \param t             bst instance
 * \param key_type      key type
 * \param value_size    size of the stored values
 */
uproc_bst *uproc_bst_create(enum uproc_bst_keytype key_type, size_t value_size);

/** Destroy BST and all contained nodes */
void uproc_bst_destroy(uproc_bst *t);

/** Return non-zero if the tree is empty */
int uproc_bst_isempty(uproc_bst *t);

/** Obtain the number of nodes */
size_t uproc_bst_size(const uproc_bst *t);

/** Insert item
 *
 * \param t     bst instance
 * \param key   search key
 * \param value pointer to value to be stored
 *
 * \retval #UPROC_SUCCESS  item was inserted
 * \retval #UPROC_EEXIST   `key` is already present
 * \retval #UPROC_ENOMEM   memory allocation failed
 */
int uproc_bst_insert(uproc_bst *t, union uproc_bst_key key,
                     const void *value);

/** Update item
 *
 * Like uproc_bst_insert(), but doesn't fail if an item with the same key was
 * already present.
 *
 * \param t     bst instance
 * \param key   search key
 * \param value pointer to value to be stored
 *
 * \retval #UPROC_SUCCESS  item was inserted/updated
 * \retval #UPROC_ENOMEM   memory allocation failed
 */
int uproc_bst_update(uproc_bst *t, union uproc_bst_key key,
                     const void *value);

/** Get item
 *
 * \param t     bst instance
 * \param key   search key
 * \param value _OUT_: value associated with `key`
 *
 * \retval #UPROC_SUCCESS  an item for `key` was found and stored in `*value`
 * \retval #UPROC_ENOENT   no item found, `*value` remains unchanged
 */
int uproc_bst_get(uproc_bst *t, union uproc_bst_key key, void *value);

/** Remove item
 *
 * \param t     BST instance
 * \param key   key of the item to remove
 * \param value _OUT_: removed value
 *
 * Returns 0 on success. If the key is not present, returns
 * ::UPROC_BST_KEY_NOT_FOUND and does not modify the object pointed to by the
 * second parameter.
 */
int uproc_bst_remove(uproc_bst *t, union uproc_bst_key key, void *value);


/** Create BST iterator */
uproc_bstiter *uproc_bstiter_create(const uproc_bst *t);

/** Obtain next key/value pair */
int uproc_bstiter_next(uproc_bstiter *iter, union uproc_bst_key *key,
                       void *value);

/** Destroy BST iterator */
void uproc_bstiter_destroy(uproc_bstiter *iter);

#endif
