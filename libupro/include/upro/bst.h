#ifndef UPRO_BST_H
#define UPRO_BST_H

/** \file upro/bst.h
 *
 * Simple binary search tree implementation
 */

#include <stdlib.h>
#include <stdint.h>
#include "upro/common.h"
#include "upro/word.h"

union upro_bst_key {
    uintmax_t uint;
    struct upro_word word;
};

enum upro_bst_keytype {
    UPRO_BST_UINT,
    UPRO_BST_WORD,
};

enum upro_bst_return {
    UPRO_BST_KEY_EXISTS = UPRO_FAILURE + 1,
    UPRO_BST_KEY_NOT_FOUND = 404,
};

/** Binary search tree */
struct upro_bst {
    /** The root node (struct upro_bst_node is defined in the implementation) */
    struct upro_bst_node *root;

    /** Number of nodes */
    size_t size;

    /** Key type */
    enum upro_bst_keytype key_type;

    /** Size of value objects */
    size_t value_size;
};

struct upro_bstiter
{
    const struct upro_bst *t;
    struct upro_bst_node *cur;
};

typedef int (*upro_bst_cb_walk)(union upro_bst_key, const void*, void*);
typedef int (*upro_bst_cb_remove)(const void*);

/** Initialize an empty binary search tree
 *
 * \param t             bst instance
 * \param key_type      key type
 * \param value_size    size of the stored values
 */
void upro_bst_init(struct upro_bst *t, enum upro_bst_keytype key_type,
        size_t data_size);

/** Remove all nodes from tree; leaves an empty tree
 *
 * Takes a callback function pointer similar to upro_bst_remove()
 *
 * \param t         bst instance
 * \param callback  callback function or null pointer
 */
void upro_bst_clear(struct upro_bst *t, upro_bst_cb_remove callback);

/** Return non-zero if the tree is empty */
int upro_bst_isempty(struct upro_bst *t);

/** Obtain the number of nodes */
size_t upro_bst_size(const struct upro_bst *t);

/** Insert item
 *
 * \param t     bst instance
 * \param key   search key
 * \param value pointer to data to be stored
 *
 * \retval #UPRO_SUCCESS  item was inserted
 * \retval #UPRO_EEXIST   `key` is already present
 * \retval #UPRO_ENOMEM   memory allocation failed
 */
int upro_bst_insert(struct upro_bst *t, union upro_bst_key key, const void *value);

/** Update item
 *
 * Like upro_bst_insert(), but doesn't fail if an item with the same key was
 * already present.
 *
 * \param t     bst instance
 * \param key   search key
 * \param value pointer to data to be stored
 *
 * \retval #UPRO_SUCCESS  item was inserted/updated
 * \retval #UPRO_ENOMEM   memory allocation failed
 */
int upro_bst_update(struct upro_bst *t, union upro_bst_key key, const void *data);

/** Get item
 *
 * \param t     bst instance
 * \param key   search key
 * \param value _OUT_: data associated with `key`
 *
 * \retval #UPRO_SUCCESS  an item for `key` was found and stored in `*value`
 * \retval #UPRO_ENOENT   no item found, `*value` remains unchanged
 */
int upro_bst_get(struct upro_bst *t, union upro_bst_key key, void *value);

/** Remove item
 *
 * Takes a callback function pointer to which the stored pointer will be
 * passed (e.g. `&free`)
 *
 * \param t         bst instance
 * \param key       key of the item to remove
 * \param callback  callback function or null pointer
 *
 * \retval #UPRO_SUCCESS  item was removed
 * \retval #UPRO_ENOENT  `key` not found in the tree
 */
int upro_bst_remove(struct upro_bst *t, union upro_bst_key key,
        upro_bst_cb_remove callback);

/** In-order iteration
 *
 * Iterate over all nodes, passing key, value and a caller-provided `opaque`
 * pointer to the function pointed to by `callback`. If the callback function
 * does _not_ return #UPRO_SUCCESS, iteration is aborted and that value is
 * returned.
 *
 * \param t         bst instance
 * \param callback  callback function pointer
 * \param opaque    third argument to `callback`
 *
 * \return
 * #UPRO_SUCCESS if the iteration completed successfully, else whatever the
 * callback function returned.
 */
int upro_bst_walk(struct upro_bst *t, upro_bst_cb_walk callback, void *opaque);

/** Initialize iterator */
void upro_bstiter_init(struct upro_bstiter *iter, const struct upro_bst *t);

/** Obtain next key/value pair */
int upro_bstiter_next(struct upro_bstiter *iter, union upro_bst_key *key, void *value);

#endif
