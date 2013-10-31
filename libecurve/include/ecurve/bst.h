#ifndef EC_BST_H
#define EC_BST_H

/** \file ecurve/bst.h
 *
 * Simple binary search tree implementation
 */

#include <stdlib.h>
#include <stdint.h>
#include "ecurve/common.h"
#include "ecurve/word.h"

union ec_bst_key {
    uintmax_t uint;
    struct ec_word word;
};

enum ec_bst_keytype {
    EC_BST_UINT,
    EC_BST_WORD,
};

/** Binary search tree */
struct ec_bst {
    /** The root node (struct ec_bst_node is defined in the implementation) */
    struct ec_bst_node *root;

    /** Number of nodes */
    size_t size;

    /** Key type */
    enum ec_bst_keytype key_type;

    /** Size of value objects */
    size_t value_size;
};

typedef int (*ec_bst_cb_walk)(union ec_bst_key, const void*, void*);
typedef int (*ec_bst_cb_remove)(const void*);

/** Initialize an empty binary search tree
 *
 * \param t             bst instance
 * \param key_type      key type
 * \param value_size    size of the stored values
 */
void ec_bst_init(struct ec_bst *t, enum ec_bst_keytype key_type,
        size_t data_size);

/** Remove all nodes from tree; leaves an empty tree
 *
 * Takes a callback function pointer similar to ec_bst_remove()
 *
 * \param t         bst instance
 * \param callback  callback function or null pointer
 */
void ec_bst_clear(struct ec_bst *t, ec_bst_cb_remove callback);

/** Return non-zero if the tree is empty */
int ec_bst_isempty(struct ec_bst *t);

/** Obtain the number of nodes */
size_t ec_bst_size(const struct ec_bst *t);

/** Insert item
 *
 * \param t     bst instance
 * \param key   search key
 * \param value pointer to data to be stored
 *
 * \retval #EC_SUCCESS  item was inserted
 * \retval #EC_EEXIST   `key` is already present
 * \retval #EC_ENOMEM   memory allocation failed
 */
int ec_bst_insert(struct ec_bst *t, union ec_bst_key key, const void *value);

/** Update item
 *
 * Like ec_bst_insert(), but doesn't fail if an item with the same key was
 * already present.
 *
 * \param t     bst instance
 * \param key   search key
 * \param value pointer to data to be stored
 *
 * \retval #EC_SUCCESS  item was inserted/updated
 * \retval #EC_ENOMEM   memory allocation failed
 */
int ec_bst_update(struct ec_bst *t, union ec_bst_key key, const void *data);

/** Get item
 *
 * \param t     bst instance
 * \param key   search key
 * \param value _OUT_: data associated with `key`
 *
 * \retval #EC_SUCCESS  an item for `key` was found and stored in `*value`
 * \retval #EC_ENOENT   no item found, `*value` remains unchanged
 */
int ec_bst_get(struct ec_bst *t, union ec_bst_key key, void *value);

/** Remove item
 *
 * Takes a callback function pointer to which the stored pointer will be
 * passed (e.g. `&free`)
 *
 * \param t         bst instance
 * \param key       key of the item to remove
 * \param callback  callback function or null pointer
 *
 * \retval #EC_SUCCESS  item was removed
 * \retval #EC_ENOENT  `key` not found in the tree
 */
int ec_bst_remove(struct ec_bst *t, union ec_bst_key key,
        ec_bst_cb_remove callback);

/** In-order iteration
 *
 * Iterate over all nodes, passing key, value and a caller-provided `opaque`
 * pointer to the function pointed to by `callback`. If the callback function
 * does _not_ return #EC_SUCCESS, iteration is aborted and that value is
 * returned.
 *
 * \param t         bst instance
 * \param callback  callback function pointer
 * \param opaque    third argument to `callback`
 *
 * \return
 * #EC_SUCCESS if the iteration completed successfully, else whatever the
 * callback function returned.
 */
int ec_bst_walk(struct ec_bst *t, ec_bst_cb_walk callback, void *opaque);


/** Free the `ptr` member of a union ec_bst_data
 *
 * Can be used as a callback for ec_bst_clear() or ec_bst_remove().
 */
int ec_bst_free_ptr(union ec_bst_data val);

#endif
