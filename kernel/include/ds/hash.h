#pragma once

#include <ds/btree.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <lib/stdint.h>
#include <sync/spinlock.h>
#include <sys/system.h>
#include <lib/string.h>

typedef unsigned long hash_key_t;

typedef struct hash_table_t {
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


#define hash_btree(h)               ({hash_assert_locked(h); (&(h)->h_btree); })
#define hash_btree_lock(h)          ({btree_lock(hash_btree(h)); })
#define hash_btree_unlock(h)        ({btree_unlock(hash_btree(h)); })
#define hash_btree_islocked(h)      ({btree_islocked(hash_btree(h)); })
#define hash_btree_assert_locked(h) ({btree_assert_locked(hash_btree(h)); })

#define HASH_INIT()                 ((hash_table_t){0})
#define HASH_NEW()                  (&HASH_INIT())

static inline void hash_init(hash_table_t *ht) {
    *ht = HASH_INIT();
}

static inline int hash_insert(hash_table_t *ht, hash_key_t (*hash_func)(), void *data) {
    int err = 0;
    hash_key_t key = 0;
    hash_node_t *head = NULL;
    hash_node_t *tail = NULL;
    hash_node_t *node = NULL;
    hash_node_t *next = NULL;

    if (ht == NULL)
        return -EINVAL;

    hash_assert_locked(ht);

    if (hash_func)
        key = (hash_func)(data);
    else
        key = (hash_key_t)data;

    if (NULL == (node = kmalloc(sizeof *node)))
        return -ENOMEM;

    node->hn_next = NULL;
    node->hn_data = data;
    node->hn_prev = NULL;

    hash_btree_lock(ht);
    if ((err = btree_search(hash_btree(ht), key, (void **)&head)) == -ENOENT) {
        err = btree_insert(hash_btree(ht), key, node);
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

static inline int hash_search(hash_table_t *ht, hash_key_t (*hash_func)(), void *data, int isstring, int (*hash_verify)(), void **pdp) {
    int err = 0;
    hash_key_t key = 0;
    hash_node_t *head = NULL, *next = NULL;

    if (ht == NULL)
        return -EINVAL;
    
    hash_assert_locked(ht);

    if (hash_func)
        key = (hash_func)(data);
    else
        key = (hash_key_t)data;

    hash_btree_lock(ht);
    if ((err = btree_search(hash_btree(ht), key, (void **)&head)) == 0) {
        forlinked(node, head, next) {
            head = node;
            if (hash_verify) {
                if (((hash_verify)(data, node->hn_data)) == 0)
                    goto found;
            } else if (isstring) {
                if (!compare_strings(node->hn_data, data))
                    goto found;
            } else if (node->hn_data == data)
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

static inline int hash_delete(hash_table_t *ht, hash_key_t (*hash_func)(), void *data, int isstring, int (*hash_verify)()) {
    int err = 0;
    hash_key_t key = 0;
    hash_node_t *prev = NULL, *next = NULL;
    hash_node_t *head = NULL, *target = NULL;

    if (ht == NULL)
        return -EINVAL;

    hash_assert_locked(ht);

    if (hash_func)
        key = (hash_func)(data);
    else
        key = (hash_key_t)data;

    hash_btree_lock(ht);
    if ((err = btree_search(hash_btree(ht), key, (void *)&head)) == 0) {
        forlinked(node, head, next) {
            target = node;
            if (hash_verify) {
                if (((hash_verify)(data, node->hn_data)) == 0)
                    goto found;
            } else if (isstring) {
                if (!compare_strings(node->hn_data, data))
                    goto found;
            } else if (node->hn_data == data)
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
        btree_delete(hash_btree(ht), key);

        /*Insert next node is available*/
        if (next) {
            if (hash_func)
                    key = (hash_func)(next->hn_data);
            else
                    key = (hash_key_t)next->hn_data;
            if ((err = btree_insert(hash_btree(ht), key, next)))
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