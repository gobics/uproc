#ifndef EC_BST_H
#define EC_BST_H

/** \file ecurve/bst.h
 *
 * Simple binary search tree implementation
 */

#include <stdint.h>

typedef struct ec_bst ec_bst;

struct ec_bst {
    struct bstnode *root;
};


/** Initialize an empty binary search tree */
void ec_bst_init(ec_bst *t);

/** Remove all nodes from tree; leaves an empty tree
 *
 * Takes a callback function pointer similar to ec_bst_remove()
 *
 * \param t         bst instance
 * \param callback  callback function or null pointer
 */
void ec_bst_clear(ec_bst *t, void (*callback)(void*));

/** Return non-zero if the tree is empty */
int ec_bst_isempty(ec_bst *t);

/** Insert item
 *
 * \param t     bst instance
 * \param key   search key
 * \param data  pointer to be stored
 *
 * \retval #EC_SUCCESS  item was inserted
 * \retval #EC_FAILURE  `key` was already present or memory allocation failed
 */
int ec_bst_insert(ec_bst *t, intmax_t key, void *data);

/** Get item
 *
 * \param t     bst instance
 * \param key   search key
 *
 * \return stored pointer, or null pointer if key not found
 */
void *ec_bst_get(ec_bst *t, intmax_t key);

/** Remove item
 *
 * Takes a callback function pointer to which the stored pointer will be
 * passed (e.g. `&free`)
 *
 * \param t         bst instance
 * \param key       key of the item to remove
 * \param callback  callback function or null pointer
 *
 * \retval #EC_SUCCESS  an item was removed
 * \retval #EC_FAILURE  `key` not found in the tree
 */
int ec_bst_remove(ec_bst *t, intmax_t key, void (*callback)(void*));

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
int ec_bst_walk(ec_bst *t, int (*callback)(intmax_t, void*, void*),
                void *opaque);

#endif
