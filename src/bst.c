#include "ecurve/common.h"
#include "ecurve/bst.h"

#include <stdlib.h>
#include <stdint.h>

struct ec_bst_node {
    intmax_t key;
    void *data;
    struct ec_bst_node *parent;
    struct ec_bst_node *left;
    struct ec_bst_node *right;
};

/* create a new bst node */
static struct ec_bst_node *bstnode_new(intmax_t key, void *data,
                                       struct ec_bst_node *parent);

/* free a bst node and all it's descendants recursively */
static void bstnode_free(struct ec_bst_node *n, void (*callback)(void*));

/* in-order iteration */
static int bstnode_walk(struct ec_bst_node *n,
                        int (*callback)(intmax_t, void*, void*), void *opaque);

/* find node in tree */
static struct ec_bst_node *bstnode_find(struct ec_bst_node *n, intmax_t key);

/* find node that replaces a node n, move it to n's position and return it */
static struct ec_bst_node *bstnode_remove(struct ec_bst_node *n);


static struct ec_bst_node *
bstnode_new(intmax_t key, void *data, struct ec_bst_node *parent)
{
    struct ec_bst_node *n = malloc(sizeof *n);

    if (n) {
        n->key = key;
        n->data = data;
        n->parent = parent;
        n->left = NULL;
        n->right = NULL;
    }
    return n;
}

static void
bstnode_free(struct ec_bst_node *n, void (*callback)(void*))
{
    if (!n) {
        return;
    }
    bstnode_free(n->left, callback);
    bstnode_free(n->right, callback);
    if (callback) {
        callback(n->data);
    }
    free(n);
}

static int
bstnode_walk(struct ec_bst_node *n, int (*callback)(intmax_t, void*, void*),
             void *opaque)
{
    int res;
    if (!n) {
        return EC_SUCCESS;
    }
    res = bstnode_walk(n->left, callback, opaque);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = (*callback)(n->key, n->data, opaque);
    if (res != EC_SUCCESS) {
        return res;
    }
    res = bstnode_walk(n->right, callback, opaque);
    return res;
}

static struct ec_bst_node *
bstnode_find(struct ec_bst_node *n, intmax_t key)
{
    if (key == n->key) {
        return n;
    }
    if (key < n->key) {
        return !n->left ? n : bstnode_find(n->left, key);
    }
    return !n->right ? n : bstnode_find(n->right, key);
}

static struct ec_bst_node *
bstnode_remove(struct ec_bst_node *n)
{
    static int del_from_left = 1;

    /* p will replace n */
    struct ec_bst_node *p;

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
ec_bst_init(struct ec_bst *t)
{
    t->root = NULL;
}

void
ec_bst_clear(struct ec_bst *t, void (*callback)(void*))
{
    if (!t || !t->root) {
        return;
    }
    bstnode_free(t->root, callback);
    t->root = NULL;
}

int
ec_bst_isempty(struct ec_bst *t)
{
    return t->root == NULL;
}

int
ec_bst_insert(struct ec_bst *t, intmax_t key, void *data)
{
    struct ec_bst_node *n, *ins;

    if (!t->root) {
        t->root = bstnode_new(key, data, NULL);
        return t->root ? EC_SUCCESS : EC_FAILURE;
    }

    n = bstnode_find(t->root, key);

    if (key == n->key) {
        return EC_FAILURE;
    }
    else if (key < n->key) {
        ins = bstnode_new(key, data, n);
        n->left = ins;
    }
    else {
        ins = bstnode_new(key, data, n);
        n->right = ins;
    }

    return ins ? EC_SUCCESS : EC_FAILURE;
}

void *
ec_bst_get(struct ec_bst *t, intmax_t key)
{
    struct ec_bst_node *n;

    if (!t || !t->root) {
        return NULL;
    }

    n = bstnode_find(t->root, key);
    return key == n->key ? n->data : NULL;
}

int
ec_bst_remove(struct ec_bst *t, intmax_t key, void (*callback)(void*))
{
    /* node to remove and its parent */
    struct ec_bst_node *del, *par;

    if (!t->root) {
        return EC_FAILURE;
    }

    del = bstnode_find(t->root, key);
    if (key != del->key) {
        return EC_FAILURE;
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
        callback(del->data);
    }
    free(del);
    return EC_SUCCESS;
}

int ec_bst_walk(struct ec_bst *t, int (*callback)(intmax_t, void*, void*),
                void *opaque)
{
    return bstnode_walk(t->root, callback, opaque);
}
