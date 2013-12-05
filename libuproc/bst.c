/* Binary search tree
 *
 * Copyright 2013 Peter Meinicke, Robin Martinjak
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
#include <stdint.h>
#include <string.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/bst.h"


struct uproc_bstnode
{
    union uproc_bst_key key;
    void *value;
    struct uproc_bstnode *parent;
    struct uproc_bstnode *left;
    struct uproc_bstnode *right;
};


/* compare keys */
static int
cmp_keys(struct uproc_bst *t, union uproc_bst_key x, union uproc_bst_key y)
{
    switch (t->key_type) {
        case UPROC_BST_UINT:
            {
                uintmax_t a = x.uint, b = y.uint;
                if (a == b) {
                    return 0;
                }
                if (a < b) {
                    return -1;
                }
                return 1;
            }
        case UPROC_BST_WORD:
            return uproc_word_cmp(&x.word, &y.word);
    }
    return uproc_error_msg(UPROC_EINVAL, "uninitialized bst");
}

/* create a new bst node */
static struct uproc_bstnode *
bstnode_new(union uproc_bst_key key, const void *value, size_t value_size,
            struct uproc_bstnode *parent)
{
    struct uproc_bstnode *n = malloc(sizeof *n);

    if (!n) {
        return NULL;
    }
    n->key = key;
    n->parent = parent;
    n->left = NULL;
    n->right = NULL;
    n->value = malloc(value_size);
    if (!n->value) {
        free(n);
        return NULL;
    }
    memcpy(n->value, value, value_size);
    return n;
}

/* free a bst node and all it's descendants recursively */
static void
bstnode_free(struct uproc_bstnode *n, size_t value_size,
             uproc_bst_cb_remove callback)
{
    if (!n) {
        return;
    }
    bstnode_free(n->left, value_size, callback);
    bstnode_free(n->right, value_size, callback);
    if (callback) {
        callback(n->value);
    }
    free(n->value);
    free(n);
}

/* in-order iteration */
static int
bstnode_walk(struct uproc_bstnode *n, uproc_bst_cb_walk callback, void *opaque)
{
    int res;
    if (!n) {
        return 0;
    }
    res = bstnode_walk(n->left, callback, opaque);
    if (res) {
        return res;
    }
    res = callback(n->key, n->value, opaque);
    if (res) {
        return res;
    }
    res = bstnode_walk(n->right, callback, opaque);
    return res;
}

/* find node in tree */
static struct uproc_bstnode *
bstnode_find(struct uproc_bst *t, struct uproc_bstnode *n,
             union uproc_bst_key key)
{
    int cmp = cmp_keys(t, key, n->key);
    if (cmp == 0) {
        return n;
    }
    if (cmp < 1) {
        return !n->left ? n : bstnode_find(t, n->left, key);
    }
    return !n->right ? n : bstnode_find(t, n->right, key);
}

/* find node that replaces a node n, move it to n's position and return it */
static struct uproc_bstnode *
bstnode_remove(struct uproc_bstnode *n)
{
    static int del_from_left = 1;

    /* p will replace n */
    struct uproc_bstnode *p;

    /* no children */
    if (!n->left && !n->right) {
        return NULL;
    }
    /* 1 child */
    else if (!n->left) {
        p = n->right;
    }
    else if (!n->right) {
        p = n->left;
    }

    /* 2 children */
    else {
        /* go either left or right way */
        del_from_left ^= 1;

        if (del_from_left) {
            /* find predecessor (rightmost item of left subtree) */
            p = n->left;
            if (p->right) {
                while (p->right) {
                    p = p->right;
                }
                /* p's left subtree becomes his former parent's right subtree
                   (p has no right subtree)
                 */
                if (p->left) {
                    p->left->parent = p->parent;
                }
                p->parent->right = p->left;
                /* p's new left subtree is n's subtree */
                p->left = n->left;
            }
            /* p's new left subtree is n's subtree */
            p->right = n->right;
        }
        /* analoguous to the above method */
        else {
            p = n->right;
            if (p->left) {
                while (p->left) {
                    p = p->left;
                }
                if (p->right) {
                    p->right->parent = p->parent;
                }
                p->parent->left = p->right;
                p->right = n->right;
            }
            p->left = n->left;
        }
    }

    if (p->right) {
        p->right->parent = p;
    }
    if (p->left) {
        p->left->parent = p;
    }
    p->parent = n->parent;
    return p;
}

void
uproc_bst_init(struct uproc_bst *t, enum uproc_bst_keytype key_type,
               size_t value_size)
{
    t->root = NULL;
    t->size = 0;
    t->key_type = key_type;
    t->value_size = value_size;
}

void
uproc_bst_clear(struct uproc_bst *t, uproc_bst_cb_remove callback)
{
    if (!t || !t->root) {
        return;
    }
    bstnode_free(t->root, t->value_size, callback);
    t->root = NULL;
    t->size = 0;
}

int
uproc_bst_isempty(struct uproc_bst *t)
{
    return t->root == NULL;
}

size_t
uproc_bst_size(const struct uproc_bst *t)
{
    return t->size;
}

static int
insert_or_update(struct uproc_bst *t, union uproc_bst_key key,
        const void *value, bool update)
{
    struct uproc_bstnode *n, *ins;
    int cmp;
    if (!t->root) {
        t->root = bstnode_new(key, value, t->value_size, NULL);
        if (t->root) {
            t->size = 1;
            return 0;
        }
        return uproc_error(UPROC_ENOMEM);
    }

    n = bstnode_find(t, t->root, key);

    cmp = cmp_keys(t, key, n->key);
    if (cmp == 0) {
        if (update) {
            memcpy(n->value, value, t->value_size);
            return 0;
        }
        return UPROC_BST_KEY_EXISTS;
    }
    else if (cmp < 0) {
        ins = bstnode_new(key, value, t->value_size, n);
        n->left = ins;
    }
    else {
        ins = bstnode_new(key, value, t->value_size, n);
        n->right = ins;
    }

    if (!ins) {
        return uproc_error(UPROC_ENOMEM);
    }

    t->size++;
    return 0;
}

int
uproc_bst_insert(struct uproc_bst *t, union uproc_bst_key key,
                 const void *value)
{
    return insert_or_update(t, key, value, false);
}

int
uproc_bst_update(struct uproc_bst *t, union uproc_bst_key key,
                 const void *value)
{
    return insert_or_update(t, key, value, true);
}

int
uproc_bst_get(struct uproc_bst *t, union uproc_bst_key key, void *value)
{
    struct uproc_bstnode *n;
    if (!t->root) {
        return UPROC_BST_KEY_NOT_FOUND;
    }

    n = bstnode_find(t, t->root, key);
    if (cmp_keys(t, key, n->key) == 0) {
        memcpy(value, n->value, t->value_size);
        return 0;
    }
    return UPROC_BST_KEY_NOT_FOUND;
}

int
uproc_bst_remove(struct uproc_bst *t, union uproc_bst_key key,
                 uproc_bst_cb_remove callback)
{
    /* node to remove and its parent */
    struct uproc_bstnode *del, *par;
    if (!t->root) {
        return UPROC_BST_KEY_NOT_FOUND;
    }

    del = bstnode_find(t, t->root, key);
    if (cmp_keys(t, key, del->key) != 0) {
        return UPROC_BST_KEY_NOT_FOUND;
    }
    par = del->parent;

    /* bstnode_remove returns node that will replace the removed node
       uptdate `del`s parents (or `t`s root if del was the old root) */
    if (del == t->root) {
        t->root = bstnode_remove(del);
    }
    else if (del == par->left) {
        par->left = bstnode_remove(del);
    }
    else {
        par->right = bstnode_remove(del);
    }

    if (callback) {
        callback(del->value);
    }
    free(del->value);
    free(del);
    t->size--;
    return 0;
}

int
uproc_bst_walk(struct uproc_bst *t, uproc_bst_cb_walk callback, void *opaque)
{
    return bstnode_walk(t->root, callback, opaque);
}

void
uproc_bstiter_init(struct uproc_bstiter *iter, const struct uproc_bst *t)
{
    struct uproc_bstnode *n = t->root;
    iter->t = t;
    if (n) {
        while (n->left) {
            n = n->left;
        }
    }
    iter->cur = n;
}

int
uproc_bstiter_next(struct uproc_bstiter *iter, union uproc_bst_key *key,
        void *value)
{
    struct uproc_bstnode *n = iter->cur;

    if (!n) {
        return 0;
    }

    *key = n->key;
    memcpy(value, n->value, iter->t->value_size);

    if (n->right) {
        n = n->right;
        while (n->left) {
            n = n->left;
        }
        iter->cur = n;
        return 1;
    }

    while (n->parent && n == n->parent->right) {
        n = n->parent;
    }
    iter->cur = n->parent;
    return 1;
}
