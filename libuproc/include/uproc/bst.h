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

/** \file uproc/bst.h
 *
 * Binary search tree
 *
 * The tree is not self-balancing, so don't use it for bigger data sets that
 * might be inserted in order.
 *
 * The keys are of type union ::uproc_bst_key, which member the tree instance used is
 * chosen via the first argument to ::uproc_bst_create. Values are stored by copying
 * the number of bytes given as the second argument to ::uproc_bst_create.
 *
 * Example:
 *
 * \code
 *  struct { int foo; double bar; } value;
 *  union uproc_bst_key key;
 *  uproc_bst *t = uproc_bst_create(UPROC_BST_UINT, sizeof value)
 *  if (!t) {
 *      // handle error
 *  }
 *
 *  key.uint = 23;
 *  value.foo = 42;
 *  value.bar = 3.14;
 *  uproc_bst_update(t, key, &value);
 *
 *  value.foo = 0;
 *  uproc_bst_get(t, key, &value);
 *  assert(value.foo == 42);
 *
 *  uproc_bst_destroy(t);
 * \endcode
 *
 *
 * \weakgroup grp_intern
 * \{
 * \weakgroup grp_intern_bst
 * \{
 */

#ifndef UPROC_BST_H
#define UPROC_BST_H

#include <stdlib.h>
#include <stdint.h>

#include "uproc/common.h"
#include "uproc/word.h"


/** \defgroup obj_bst object uproc_bst
 * \{
 */

/** Opaque type for binary search trees */
typedef struct uproc_bst_s uproc_bst;

/** BST key type */
union uproc_bst_key
{
    /** Unsigned integer */
    uintmax_t uint;

    /** Amino acid word */
    struct uproc_word word;
};


/** Which member of union ::uproc_bst_key is used */
enum uproc_bst_keytype
{
    /** .uint (uintmax_t) */
    UPROC_BST_UINT,

    /** .word (struct uproc_word) */
    UPROC_BST_WORD,
};


/** BST specific return codes */
enum
{
    /** BST doesn't contain an item with the given key */

    UPROC_BST_KEY_NOT_FOUND = -404,

    /** BST already contains an item with the given key. */
    UPROC_BST_KEY_EXISTS = -403,
};






/** Initialize an empty binary search tree
 *
 * \param key_type      key type
 * \param value_size    size of the stored values
 */
uproc_bst *uproc_bst_create(enum uproc_bst_keytype key_type, size_t value_size);


/** Destroy BST and all contained nodes */
void uproc_bst_destroy(uproc_bst *t);


/** Return non-zero if the tree is empty */
int uproc_bst_isempty(uproc_bst *t);


/** Return the number of nodes */
size_t uproc_bst_size(const uproc_bst *t);


/** Insert item
 *
 * \param t     BST instance
 * \param key   search key
 * \param value pointer to value to be stored
 *
 * \return
 * Returns 0 on success, ::UPROC_BST_KEY_EXISTS if the key is already present.
 * If memory allocation fails, returns -1 and sets ::uproc_errno to
 * ::UPROC_ENOMEM.
 */
int uproc_bst_insert(uproc_bst *t, union uproc_bst_key key,
                     const void *value);


/** Insert or update item
 *
 * Like uproc_bst_insert(), but overwrites the value of an already present key.
 *
 * \param t     BST instance
 * \param key   search key
 * \param value pointer to value to be stored
 */
int uproc_bst_update(uproc_bst *t, union uproc_bst_key key,
                     const void *value);


/** Get item
 *
 * \param t     BST instance
 * \param key   search key
 * \param value _OUT_: value associated with the key
 *
 * \return
 * Returns 0 on success. If the key is not present, returns
 * ::UPROC_BST_KEY_NOT_FOUND and does not modify the object pointed to by the
 * second parameter.
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

/** \} */


/** \defgroup obj_bstiter object uproc_bstiter
 * \{
 */

/** BST in-order iterator */
typedef struct uproc_bstiter_s uproc_bstiter;

/** Create BST in-order iterator
 *
 * If the tree being iterated is modified, correct behaviour of the iterator is
 * not guaranteed (i.o.w. don't do it).
 *
 * \param t     BST instance
 *
 * \return
 * Returns a new BST iterator instance.
 * If memory allocation fails, returns NULL and sets ::uproc_errno to
 * ::UPROC_ENOMEM.
 */
uproc_bstiter *uproc_bstiter_create(const uproc_bst *t);


/** Obtain next key/value pair
 *
 *  \param iter     BST iterator
 *  \param key      _OUT_: value of next key
 *  \param value    _OUT_: value of next item
 *
 *  \return
 *  Returns 0 if an item was produced.
 *  If the iterator is exhausted, returns 1 and does not modify *key or *value.
 *  This function can not fail, so -1 is never returned.
 */
int uproc_bstiter_next(uproc_bstiter *iter, union uproc_bst_key *key,
                       void *value);

/** Destroy BST iterator */
void uproc_bstiter_destroy(uproc_bstiter *iter);

/** \} */

/**
 * \}
 * \}
 */
#endif
