#include "ecurve/common.h"
#include "ecurve/bst.h"

#include <stdlib.h>
#include <stdint.h>

struct ec_bst_node {
    union ec_bst_key key;
    void *data;
    struct ec_bst_node *parent;
    struct ec_bst_node *left;
    struct ec_bst_node *right;
};

static int cmp_uint(union ec_bst_key x, union ec_bst_key y);

static int cmp_word(union ec_bst_key x, union ec_bst_key y);

/* create a new bst node */
static struct ec_bst_node *bstnode_new(union ec_bst_key key, void *data,
        struct ec_bst_node *parent);

/* free a bst node and all it's descendants recursively */
static void bstnode_free(struct ec_bst_node *n, void (*callback)(void*));

/* in-order iteration */
static int bstnode_walk(struct ec_bst_node *n,
        int (*callback)(union ec_bst_key, void*, void*), void *opaque);

/* find node in tree */
static struct ec_bst_node *bstnode_find(struct ec_bst *t,
        struct ec_bst_node *n, union ec_bst_key key);

/* find node that replaces a node n, move it to n's position and return it */
static struct ec_bst_node *bstnode_remove(struct ec_bst_node *n);


/* compare unsigned integers */
static int cmp_uint(union ec_bst_key x, union ec_bst_key y)
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

/* compare ec_word values */
static int cmp_word(union ec_bst_key x, union ec_bst_key y)
{
    return ec_word_cmp(&x.word, &y.word);
}

static struct ec_bst_node *
bstnode_new(union ec_bst_key key, void *data, struct ec_bst_node *parent)
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
bstnode_walk(struct ec_bst_node *n, int (*callback)(union ec_bst_key, void*, void*),
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
bstnode_find(struct ec_bst *t, struct ec_bst_node *n, union ec_bst_key key)
{
    int cmp = t->cmp(key, n->key);
    if (cmp == 0) {
        return n;
    }
    if (cmp < 1) {
        return !n->left ? n : bstnode_find(t, n->left, key);
    }
    return !n->right ? n : bstnode_find(t, n->right, key);
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
ec_bst_init(struct ec_bst *t, enum ec_bst_type type)
{
    t->root = NULL;
    t->size = 0;
    switch (type) {
        case EC_BST_WORD:
            t->cmp = cmp_word;
            break;

        case EC_BST_UINT:
        default:
            t->cmp = cmp_uint;
            break;
    }
}

void
ec_bst_clear(struct ec_bst *t, void (*callback)(void*))
{
    if (!t || !t->root) {
        return;
    }
    bstnode_free(t->root, callback);
    t->root = NULL;
    t->size = 0;
}

int
ec_bst_isempty(struct ec_bst *t)
{
    return t->root == NULL;
}

size_t
ec_bst_size(const struct ec_bst *t)
{
    return t->size;
}

int
ec_bst_insert(struct ec_bst *t, union ec_bst_key key, void *data)
{
    struct ec_bst_node *n, *ins;
    int cmp;

    if (!t->root) {
        t->root = bstnode_new(key, data, NULL);
        t->size = 1;
        return t->root ? EC_SUCCESS : EC_FAILURE;
    }

    n = bstnode_find(t, t->root, key);

    cmp = t->cmp(key, n->key);
    if (cmp == 0) {
        return EC_FAILURE;
    }
    else if (cmp < 0) {
        ins = bstnode_new(key, data, n);
        n->left = ins;
    }
    else {
        ins = bstnode_new(key, data, n);
        n->right = ins;
    }

    if (!ins) {
        return EC_FAILURE;
    }

    t->size++;
    return EC_SUCCESS;
}

#define INSERT(member, type) \
int                                                                         \
ec_bst_insert_ ## member(struct ec_bst *t, type key, void *data)            \
{                                                                           \
    union ec_bst_key k = { .member = key };                                 \
    return ec_bst_insert(t, k, data);                                       \
}

void *
ec_bst_get(struct ec_bst *t, union ec_bst_key key)
{
    struct ec_bst_node *n;

    if (!t || !t->root) {
        return NULL;
    }

    n = bstnode_find(t, t->root, key);
    return t->cmp(key, n->key) == 0 ? n->data : NULL;
}

#define GET(member, type) \
void *                                                                      \
ec_bst_get_ ## member(struct ec_bst *t, type key)                           \
{                                                                           \
    union ec_bst_key k = { .member = key };                                 \
    return ec_bst_get(t, k);                                                \
}

int
ec_bst_remove(struct ec_bst *t, union ec_bst_key key, void (*callback)(void*))
{
    /* node to remove and its parent */
    struct ec_bst_node *del, *par;

    if (!t->root) {
        return EC_FAILURE;
    }

    del = bstnode_find(t, t->root, key);
    if (t->cmp(key, del->key) != 0) {
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
    t->size--;
    return EC_SUCCESS;
}

#define REMOVE(member, type) \
int                                                                         \
ec_bst_remove_ ## member(struct ec_bst *t, type key,                        \
        void (*callback)(void*))                                            \
{                                                                           \
    union ec_bst_key k = { .member = key };                                 \
    return ec_bst_remove(t, k, callback);                                   \
}

#define WRAP(member, type)  \
    INSERT(member, type)    \
    GET(member, type)       \
    REMOVE(member, type)

WRAP(word, struct ec_word)
WRAP(uint, uintmax_t)

int ec_bst_walk(struct ec_bst *t,
        int (*callback)(union ec_bst_key, void*, void*), void *opaque)
{
    return bstnode_walk(t->root, callback, opaque);
}
