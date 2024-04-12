#pragma once

#include <ds/btree.h>
#include <ds/queue.h>
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

extern int  hash_free(hash_table_t *ht);
extern void hash_destroy(hash_table_t *ht);
extern void hash_init(hash_table_t *ht, hash_ctx_t *ctx);
extern int  hash_traverse(hash_table_t *ht, queue_t *queue);
extern int  hash_alloc(hash_ctx_t *ctx, hash_table_t **phtp);
extern int  hash_insert(hash_table_t *ht, void *key, void *data);
extern int  hash_delete(hash_table_t *ht, void *key, int isstring);
extern int  hash_traverse_btree_node(btree_node_t *tree, queue_t *queue);
extern int  hash_search(hash_table_t *ht, void *key, int isstring, void **pdp);