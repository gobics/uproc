#include "upro/common.h"
#include "upro/bst.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct upro_bst_node {
    union upro_bst_key key;
    void *value;
    struct upro_bst_node *parent;
    struct upro_bst_node *left;
    struct upro_bst_node *right;
};

/* compare keys */
static int cmp_keys(struct upro_bst *t, union upro_bst_key x, union upro_bst_key y);

/* create a new bst node */
static struct upro_bst_node *bstnode_new(union upro_bst_key key,
        const void *value, size_t value_size,
        struct upro_bst_node *parent);

/* free a bst node and all it's descendants recursively */
static void bstnode_free(struct upro_bst_node *n, size_t value_size,
        upro_bst_cb_remove callback);

/* in-order iteration */
static int bstnode_walk(struct upro_bst_node *n, upro_bst_cb_walk callback,
        void *opaque);

/* find node in tree */
static struct upro_bst_node *bstnode_find(struct upro_bst *t,
        struct upro_bst_node *n, union upro_bst_key key);

/* find node that replaces a node n, move it to n's position and return it */
static struct upro_bst_node *bstnode_remove(struct upro_bst_node *n);


static int
cmp_keys(struct upro_bst *t, union upro_bst_key x, union upro_bst_key y)
{
    switch (t->key_type) {
        case UPRO_BST_UINT:
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
        case UPRO_BST_WORD:
            return upro_word_cmp(&x.word, &y.word);
    }
    abort();
    return 42;
}

static struct upro_bst_node *
bstnode_new(union upro_bst_key key, const void *value, size_t value_size,
        struct upro_bst_node *parent)
{
    struct upro_bst_node *n = malloc(sizeof *n);

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

static void
bstnode_free(struct upro_bst_node *n, size_t value_size, upro_bst_cb_remove callback)
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

static int
bstnode_walk(struct upro_bst_node *n, upro_bst_cb_walk callback, void *opaque)
{
    int res;
    if (!n) {
        return UPRO_SUCCESS;
    }
    res = bstnode_walk(n->left, callback, opaque);
    if (res != UPRO_SUCCESS) {
        return res;
    }
    res = callback(n->key, n->value, opaque);
    if (res != UPRO_SUCCESS) {
        return res;
    }
    res = bstnode_walk(n->right, callback, opaque);
    return res;
}

static struct upro_bst_node *
bstnode_find(struct upro_bst *t, struct upro_bst_node *n, union upro_bst_key key)
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

static struct upro_bst_node *
bstnode_remove(struct upro_bst_node *n)
{
    static int del_from_left = 1;

    /* p will replace n */
    struct upro_bst_node *p;

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
upro_bst_init(struct upro_bst *t, enum upro_bst_keytype key_type, size_t value_size)
{
    t->root = NULL;
    t->size = 0;
    t->key_type = key_type;
    t->value_size = value_size;
}

void
upro_bst_clear(struct upro_bst *t, upro_bst_cb_remove callback)
{
    if (!t || !t->root) {
        return;
    }
    bstnode_free(t->root, t->value_size, callback);
    t->root = NULL;
    t->size = 0;
}

int
upro_bst_isempty(struct upro_bst *t)
{
    return t->root == NULL;
}

size_t
upro_bst_size(const struct upro_bst *t)
{
    return t->size;
}

static int
insert_or_update(struct upro_bst *t, union upro_bst_key key,
        const void *value, bool update)
{
    struct upro_bst_node *n, *ins;
    int cmp;
    if (!t->root) {
        t->root = bstnode_new(key, value, t->value_size, NULL);
        if (t->root) {
            t->size = 1;
            return UPRO_SUCCESS;
        }
        return UPRO_ENOMEM;
    }

    n = bstnode_find(t, t->root, key);

    cmp = cmp_keys(t, key, n->key);
    if (cmp == 0) {
        if (update) {
            memcpy(n->value, value, t->value_size);
            return UPRO_SUCCESS;
        }
        return UPRO_EEXIST;
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
        return UPRO_ENOMEM;
    }

    t->size++;
    return UPRO_SUCCESS;
}

int
upro_bst_insert(struct upro_bst *t, union upro_bst_key key, const void *value)
{
    return insert_or_update(t, key, value, false);
}

int
upro_bst_update(struct upro_bst *t, union upro_bst_key key, const void *value)
{
    return insert_or_update(t, key, value, true);
}

int
upro_bst_get(struct upro_bst *t, union upro_bst_key key, void *value)
{
    struct upro_bst_node *n;
    if (!t->root) {
        return UPRO_ENOENT;
    }

    n = bstnode_find(t, t->root, key);
    if (cmp_keys(t, key, n->key) == 0) {
        memcpy(value, n->value, t->value_size);
        return UPRO_SUCCESS;
    }
    return UPRO_ENOENT;
}

int
upro_bst_remove(struct upro_bst *t, union upro_bst_key key, upro_bst_cb_remove callback)
{
    /* node to remove and its parent */
    struct upro_bst_node *del, *par;
    if (!t->root) {
        return UPRO_ENOENT;
    }

    del = bstnode_find(t, t->root, key);
    if (cmp_keys(t, key, del->key) != 0) {
        return UPRO_ENOENT;
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
    return UPRO_SUCCESS;
}

int
upro_bst_walk(struct upro_bst *t, upro_bst_cb_walk callback, void *opaque)
{
    return bstnode_walk(t->root, callback, opaque);
}

void
upro_bstiter_init(struct upro_bstiter *iter, const struct upro_bst *t)
{
    struct upro_bst_node *n = t->root;
    iter->t = t;
    if (n) {
        while (n->left) {
            n = n->left;
        }
    }
    iter->cur = n;
}

int
upro_bstiter_next(struct upro_bstiter *iter, union upro_bst_key *key,
        void *value)
{
    struct upro_bst_node *n = iter->cur;

    if (!n) {
        return UPRO_ITER_STOP;
    }

    *key = n->key;
    memcpy(value, n->value, iter->t->value_size);

    if (n->right) {
        n = n->right;
        while (n->left) {
            n = n->left;
        }
        iter->cur = n;
        return UPRO_ITER_YIELD;
    }

    while (n->parent && n == n->parent->right) {
        n = n->parent;
    }
    iter->cur = n->parent;
    return UPRO_ITER_YIELD;
}
