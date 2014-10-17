/* Binary search tree
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
#include <stdint.h>
#include <string.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/bst.h"

struct bstnode
{
    union uproc_bst_key key;
    struct bstnode *parent;
    struct bstnode *left;
    struct bstnode *right;
    unsigned char value[];
};

struct uproc_bst_s
{
    /** The root node */
    struct bstnode *root;

    /** Number of nodes */
    size_t size;

    /** Key type */
    enum uproc_bst_keytype key_type;

    /** Size of value objects */
    size_t value_size;
};

struct uproc_bstiter_s
{
    /** BST being iterated */
    const struct uproc_bst_s *t;

    /** Current position */
    struct bstnode *cur;
};

/* compare keys */
static int cmp_keys(struct uproc_bst_s *t, union uproc_bst_key x,
                    union uproc_bst_key y)
{
    switch (t->key_type) {
        case UPROC_BST_UINT: {
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
static struct bstnode *bstnode_create(union uproc_bst_key key,
                                      const void *value, size_t value_size,
                                      struct bstnode *parent)
{
    struct bstnode *n = malloc(sizeof *n + value_size);

    if (!n) {
        return NULL;
    }
    n->key = key;
    n->parent = parent;
    n->left = NULL;
    n->right = NULL;
    memcpy(n->value, value, value_size);
    return n;
}

/* free a bst node and all it's descendants recursively */
static void bstnode_free(struct bstnode *n, size_t value_size)
{
    if (!n) {
        return;
    }
    bstnode_free(n->left, value_size);
    bstnode_free(n->right, value_size);
    free(n);
}

/* find node in tree */
static struct bstnode *bstnode_find(struct uproc_bst_s *t, struct bstnode *n,
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
static struct bstnode *bstnode_remove(struct bstnode *n)
{
    static int del_from_left = 1;

    /* p will replace n */
    struct bstnode *p;

    /* no children */
    if (!n->left && !n->right) {
        return NULL;
    }
    /* 1 child */
    else if (!n->left) {
        p = n->right;
    } else if (!n->right) {
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

static void bstnode_map(struct bstnode *n,
                        void (*func)(union uproc_bst_key, void *, void *),
                        void *opaque)
{
    if (!n) {
        return;
    }
    bstnode_map(n->left, func, opaque);
    func(n->key, n->value, opaque);
    bstnode_map(n->right, func, opaque);
}

uproc_bst *uproc_bst_create(enum uproc_bst_keytype key_type, size_t value_size)
{
    struct uproc_bst_s *t = malloc(sizeof *t);
    if (!t) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    t->root = NULL;
    t->size = 0;
    t->key_type = key_type;
    t->value_size = value_size;
    return t;
}

void uproc_bst_destroy(uproc_bst *t)
{
    if (!t) {
        return;
    }
    bstnode_free(t->root, t->value_size);
    free(t);
}

int uproc_bst_isempty(uproc_bst *t)
{
    return t->root == NULL;
}

size_t uproc_bst_size(const uproc_bst *t)
{
    return t->size;
}

static int insert_or_update(struct uproc_bst_s *t, union uproc_bst_key key,
                            const void *value, bool update)
{
    struct bstnode *n, *ins;
    int cmp;
    if (!t->root) {
        t->root = bstnode_create(key, value, t->value_size, NULL);
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
    } else if (cmp < 0) {
        ins = bstnode_create(key, value, t->value_size, n);
        n->left = ins;
    } else {
        ins = bstnode_create(key, value, t->value_size, n);
        n->right = ins;
    }

    if (!ins) {
        return uproc_error(UPROC_ENOMEM);
    }

    t->size++;
    return 0;
}

int uproc_bst_insert(uproc_bst *t, union uproc_bst_key key, const void *value)
{
    return insert_or_update(t, key, value, false);
}

int uproc_bst_update(uproc_bst *t, union uproc_bst_key key, const void *value)
{
    return insert_or_update(t, key, value, true);
}

int uproc_bst_get(uproc_bst *t, union uproc_bst_key key, void *value)
{
    struct bstnode *n;
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

int uproc_bst_remove(uproc_bst *t, union uproc_bst_key key, void *value)
{
    /* node to remove and its parent */
    struct bstnode *del, *par;
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
    } else if (del == par->left) {
        par->left = bstnode_remove(del);
    } else {
        par->right = bstnode_remove(del);
    }

    memcpy(value, del->value, t->value_size);
    free(del);
    t->size--;
    return 0;
}

void uproc_bst_map(const uproc_bst *t,
                   void (*func)(union uproc_bst_key, void *, void *),
                   void *opaque)
{
    bstnode_map(t->root, func, opaque);
}

uproc_bstiter *uproc_bstiter_create(const uproc_bst *t)
{
    struct bstnode *n = t->root;
    struct uproc_bstiter_s *iter = malloc(sizeof *iter);
    if (!iter) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    iter->t = t;
    if (n) {
        while (n->left) {
            n = n->left;
        }
    }
    iter->cur = n;
    return iter;
}

int uproc_bstiter_next(uproc_bstiter *iter, union uproc_bst_key *key,
                       void *value)
{
    struct bstnode *n = iter->cur;

    if (!n) {
        return 1;
    }

    *key = n->key;
    memcpy(value, n->value, iter->t->value_size);

    if (n->right) {
        n = n->right;
        while (n->left) {
            n = n->left;
        }
        iter->cur = n;
        return 0;
    }

    while (n->parent && n == n->parent->right) {
        n = n->parent;
    }
    iter->cur = n->parent;
    return 0;
}

void uproc_bstiter_destroy(uproc_bstiter *iter)
{
    free(iter);
}
