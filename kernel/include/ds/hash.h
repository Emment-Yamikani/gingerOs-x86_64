#pragma once

#include <ds/btree.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <lib/stdint.h>
#include <sync/spinlock.h>
#include <sys/system.h>
#include <lib/string.h>

typedef unsigned long hash_key_t;

typedef struct hash_ctx_t {
    int (*hash_verify_obj)();
    hash_key_t (*hash_func)();
} hash_ctx_t;

typedef struct hash_table_t {
    hash_ctx_t  *h_ctx;
    btree_t     h_btree;
    spinlock_t  h_spinlock;
} hash_table_t;

typedef struct hash_node_t {
    struct hash_node_t *hn_prev;
    void               *hn_data;
    struct hash_node_t *hn_next;
} hash_node_t;

#define hash_assert(h)              ({assert((h), "No Hash Table."); })
#define hash_lock(h)                ({hash_assert(h); spin_lock(&(h)->h_spinlock); })
#define hash_unlock(h)              ({hash_assert(h); spin_unlock(&(h)->h_spinlock); })
#define hash_islocked(h)            ({hash_assert(h); spin_islocked(&(h)->h_spinlock); })
#define hash_assert_locked(h)       ({hash_assert(h); spin_assert_locked(&(h)->h_spinlock); })


#define hash_ctx(h)                 ({hash_assert_locked(h); ((h)->h_ctx); })
#define hash_btree(h)               ({hash_assert_locked(h); (&(h)->h_btree); })
#define hash_btree_lock(h)          ({btree_lock(hash_btree(h)); })
#define hash_btree_unlock(h)        ({btree_unlock(hash_btree(h)); })
#define hash_btree_islocked(h)      ({btree_islocked(hash_btree(h)); })
#define hash_btree_assert_locked(h) ({btree_assert_locked(hash_btree(h)); })

#define HASH_INIT(ctx) ((hash_table_t){ \
    .h_ctx = (ctx),                     \
    .h_btree = {0},                     \
    .h_spinlock = SPINLOCK_INIT(),      \
})

#define HASH_NEW(ctx)   (HASH_INIT(ctx))

static inline void hash_init(hash_table_t *ht, hash_ctx_t *ctx) {
    *ht = HASH_INIT(ctx);
}

static inline int hash_insert(hash_table_t *ht, void *key, void *data) {
    int err = 0;
    hash_key_t hashed_key = 0;
    hash_node_t *head = NULL;
    hash_node_t *tail = NULL;
    hash_node_t *node = NULL;
    hash_node_t *next = NULL;

    if (ht == NULL)
        return -EINVAL;

    hash_assert_locked(ht);
    
    if (hash_ctx(ht) == NULL)
        return -EINVAL;

    if (hash_ctx(ht)->hash_func)
        hashed_key = (hash_ctx(ht)->hash_func)(key);
    else
        hashed_key = (hash_key_t)key;

    if (NULL == (node = kmalloc(sizeof *node)))
        return -ENOMEM;

    node->hn_next = NULL;
    node->hn_data = data;
    node->hn_prev = NULL;

    hash_btree_lock(ht);
    if ((err = btree_search(hash_btree(ht), hashed_key, (void **)&head)) == -ENOENT) {
        err = btree_insert(hash_btree(ht), hashed_key, node);
    } else if (err == 0) {
        forlinked(node, head, next)
            next = (tail = node)->hn_next;
        
        tail->hn_next = node;
        node->hn_prev = tail;
    }
    hash_btree_unlock(ht);

    if (err) goto error;

    return 0;
error:
    if (node)
        kfree(node);
    return err;
}

static inline int hash_search(hash_table_t *ht, void *key, int isstring, void **pdp) {
    int err = 0;
    hash_key_t hashed_key = 0;
    hash_node_t *head = NULL, *next = NULL;

    if (ht == NULL)
        return -EINVAL;
    
    hash_assert_locked(ht);

    if (hash_ctx(ht) == NULL)
        return -EINVAL;

    if (hash_ctx(ht)->hash_func)
        hashed_key = (hash_ctx(ht)->hash_func)(key);
    else
        hashed_key = (hash_key_t)key;

    hash_btree_lock(ht);
    if ((err = btree_search(hash_btree(ht), hashed_key, (void **)&head)) == 0) {
        forlinked(node, head, next) {
            head = node;
            if (hash_ctx(ht)->hash_verify_obj) {
                if (((hash_ctx(ht)->hash_verify_obj)(key, node->hn_data)) == 0)
                    goto found;
            } else if (isstring) {
                if (!compare_strings(node->hn_data, key))
                    goto found;
            } else if (node->hn_data == key)
                    goto found;
            next = node->hn_next;
        }
        err = -ENOENT;
    }
    hash_btree_unlock(ht);
    return err;
found:
    if (pdp)
        *pdp = head->hn_data;
    hash_btree_unlock(ht);
    return 0;
}

static inline int hash_delete(hash_table_t *ht, void *key, int isstring) {
    int err = 0;
    hash_key_t hashed_key = 0;
    hash_node_t *prev = NULL, *next = NULL;
    hash_node_t *head = NULL, *target = NULL;

    if (ht == NULL)
        return -EINVAL;

    hash_assert_locked(ht);

    if (hash_ctx(ht) == NULL)
        return -EINVAL;

    if (hash_ctx(ht)->hash_func)
        hashed_key = (hash_ctx(ht)->hash_func)(key);
    else
        hashed_key = (hash_key_t)key;

    hash_btree_lock(ht);
    if ((err = btree_search(hash_btree(ht), hashed_key, (void *)&head)) == 0) {
        forlinked(node, head, next) {
            target = node;
            if (hash_ctx(ht)->hash_verify_obj) {
                if (((hash_ctx(ht)->hash_verify_obj)(key, node->hn_data)) == 0)
                    goto found;
            } else if (isstring) {
                if (!compare_strings(node->hn_data, key))
                    goto found;
            } else if (node->hn_data == key)
                    goto found;
            next = node->hn_next;
        }
        err = -ENOENT;
    }
    hash_btree_unlock(ht);
    return err;
found:
    prev = target->hn_prev;
    next = target->hn_next;
    if (next)
        next->hn_prev = prev;

    if (prev)
        prev->hn_next = next;
    
    if (target == head) {
        /*Remove the head node*/
        btree_delete(hash_btree(ht), hashed_key);

        /*Insert next node is available*/
        if (next) {
            if (hash_ctx(ht)->hash_func)
                    hashed_key = (hash_ctx(ht)->hash_func)(next->hn_data);
            else
                    hashed_key = (hash_key_t)next->hn_data;
            if ((err = btree_insert(hash_btree(ht), hashed_key, next)))
                goto error;
        }
    }
    hash_btree_unlock(ht);
    
    kfree(target);
    return 0;
error:
    hash_btree_unlock(ht);
    return err;
}

static inline int hash_alloc(hash_ctx_t *ctx, hash_table_t **phtp) {
    hash_table_t *ht = NULL;
    
    if (ctx == NULL || phtp == NULL)
        return -EINVAL;

    if ((ht = kmalloc(sizeof *ht)) == NULL)
        return -ENOMEM;
    
    hash_init(ht, ctx);

    *phtp = ht;
    return 0;
}

static inline void hash_destroy(hash_table_t *ht) {
    if (!hash_islocked(ht))
        hash_lock(ht);
    hash_unlock(ht);
    kfree(ht);
}

static inline int hash_free(hash_table_t *ht) {
    hash_assert_locked(ht);
    if (!btree_isempty(hash_btree(ht)))
        return -ENOTEMPTY;
    hash_destroy(ht);
    return 0;
}
