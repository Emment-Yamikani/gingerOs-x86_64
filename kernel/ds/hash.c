#include <ds/btree.h>
#include <ds/queue.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <lib/stdint.h>
#include <sync/spinlock.h>
#include <sys/system.h>
#include <lib/string.h>
#include <ds/hash.h>

void hash_init(hash_table_t *ht, hash_ctx_t *ctx) {
    *ht = HASH_INIT(ctx);
}

int hash_insert(hash_table_t *ht, void *key, void *data) {
    int         err     = 0;
    hash_key_t  hsh_key = 0;
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
        hsh_key = (hash_ctx(ht)->hash_func)(key);
    else
        hsh_key = (hash_key_t)key;

    if (NULL == (node = kmalloc(sizeof *node)))
        return -ENOMEM;

    node->hn_next = NULL;
    node->hn_data = data;
    node->hn_prev = NULL;

    hash_btree_lock(ht);
    if ((err = btree_search(hash_btree(ht), hsh_key, (void **)&head)) == -ENOENT) {
        err = btree_insert(hash_btree(ht), hsh_key, node);
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

int hash_search(hash_table_t *ht, void *key, int isstring, void **pdp) {
    int err = 0;
    hash_key_t hsh_key = 0;
    hash_node_t *head = NULL, *next = NULL;

    if (ht == NULL)
        return -EINVAL;
    
    hash_assert_locked(ht);

    if (hash_ctx(ht) == NULL)
        return -EINVAL;

    if (hash_ctx(ht)->hash_func)
        hsh_key = (hash_ctx(ht)->hash_func)(key);
    else
        hsh_key = (hash_key_t)key;

    hash_btree_lock(ht);
    if ((err = btree_search(hash_btree(ht), hsh_key, (void **)&head)) == 0) {
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

int hash_delete(hash_table_t *ht, void *key, int isstring) {
    int         err     = 0;
    hash_key_t  hsh_key = 0;
    hash_node_t *prev   = NULL, *next = NULL;
    hash_node_t *head   = NULL, *target = NULL;

    if (ht == NULL)
        return -EINVAL;

    hash_assert_locked(ht);

    if (hash_ctx(ht) == NULL)
        return -EINVAL;

    if (hash_ctx(ht)->hash_func)
        hsh_key = (hash_ctx(ht)->hash_func)(key);
    else
        hsh_key = (hash_key_t)key;

    hash_btree_lock(ht);
    if ((err = btree_search(hash_btree(ht), hsh_key, (void *)&head)) == 0) {
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
        btree_delete(hash_btree(ht), hsh_key);

        /*Insert next node is available*/
        if (next) {
            if (hash_ctx(ht)->hash_func)
                    hsh_key = (hash_ctx(ht)->hash_func)(next->hn_data);
            else
                    hsh_key = (hash_key_t)next->hn_data;
            if ((err = btree_insert(hash_btree(ht), hsh_key, next)))
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

int hash_traverse_btree_node(btree_node_t *tree, queue_t *queue) {
    int err = 0;
    hash_node_t *hnode = NULL;
    queue_assert_locked(queue);

    if (queue == NULL)
        return -EINVAL;

    if (tree) {
        btree_assert_locked(tree->btree);
        hash_traverse_btree_node(tree->left, queue);
        hnode = tree->data;
        forlinked(node, hnode, node->hn_next) {
            if ((err = enqueue(queue, node->hn_data, 1, NULL)))
                return err;
        }
        hash_traverse_btree_node(tree->right, queue);
    }

    return 0;
}

int hash_traverse(hash_table_t *ht, queue_t *queue) {
    int err = 0;
    hash_assert_locked(ht);
    queue_assert_locked(queue);

    btree_lock(hash_btree(ht));
    err = hash_traverse_btree_node(hash_btree(ht)->root, queue);
    btree_unlock(hash_btree(ht));
    return err;
}

int hash_alloc(hash_ctx_t *ctx, hash_table_t **phtp) {
    hash_table_t *ht = NULL;
    
    if (ctx == NULL || phtp == NULL)
        return -EINVAL;

    if ((ht = kmalloc(sizeof *ht)) == NULL)
        return -ENOMEM;
    
    hash_init(ht, ctx);

    *phtp = ht;
    return 0;
}

void hash_destroy(hash_table_t *ht) {
    if (!hash_islocked(ht))
        hash_lock(ht);
    hash_unlock(ht);
    kfree(ht);
}

int hash_free(hash_table_t *ht) {
    hash_assert_locked(ht);
    if (!btree_isempty(hash_btree(ht)))
        return -ENOTEMPTY;
    hash_destroy(ht);
    return 0;
}