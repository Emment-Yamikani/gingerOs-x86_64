#include <bits/errno.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <ds/queue.h>

int queue_alloc(queue_t **pqp) {
    queue_t *q = NULL;

    if (pqp == NULL)
        return -EINVAL;

    if ((q = kmalloc(sizeof *q)) == NULL)
        return -ENOMEM;

    memset(q, 0, sizeof *q);
    q->q_lock = SPINLOCK_INIT();
    *pqp = q;

    return 0;
}

void queue_flush(queue_t *q)
{
    queue_node_t *next = NULL, *prev = NULL;
    queue_assert_locked(q);

    forlinked(node, q->head, next)
    {
        next = node->next;
        prev = node->prev;
        if (prev)
            prev->next = next;
        if (next)
            next->prev = prev;
        if (node == q->head)
            q->head = next;
        if (node == q->tail)
            q->tail = prev;

        q->q_count--;
        node->queue = NULL;
        kfree(node);
    }
}

void queue_free(queue_t *q)
{
    if (!queue_islocked(q))
        queue_lock(q);
    queue_flush(q);
    queue_unlock(q);
    kfree(q);
}

size_t queue_count(queue_t *q)
{
    queue_assert_locked(q);
    return q->q_count;
}

int queue_peek(queue_t *q, int tail, void **pdp) {
    queue_assert_locked(q);

    if (q == NULL || pdp == NULL)
        return -EINVAL;
    
    if (queue_count(q) == 0)
        return -ENOENT;
    
    if (tail)
        *pdp = q->tail->data;
    else
        *pdp = q->head->data;
    
    return 0;
}

int queue_contains(queue_t *q, void *data, queue_node_t **pnp) {
    queue_node_t *next = NULL;
    queue_assert_locked(q);
    if (q == NULL)
        return -EINVAL;

    forlinked(node, q->head, next)
    {
        next = node->next;
        if (node->data == data)
        {
            if (pnp)
                *pnp = node;
            return 0;
        }
    }

    return -ENOENT;
}

int enqueue(queue_t *q, void *data, int unique, queue_node_t **pnp) {
    int err = 0;
    queue_node_t *node = NULL;
    queue_assert_locked(q);

    if (q == NULL)
        return -EINVAL;

    if (unique){
        if ((err = queue_contains(q, data, NULL)) == 0)
            return -EEXIST;
    }

    if ((node = kmalloc(sizeof(*node))) == NULL)
        return -ENOMEM;

    memset(node, 0, sizeof *node);

    node->data = data;

    if (q->head == NULL)
        q->head = node;
    else
    {
        q->tail->next = node;
        node->prev = q->tail;
    }

    q->tail = node;
    node->queue = q;
    q->q_count++;

    if (pnp)
        *pnp = node;

    return 0;
}

int enqueue_head(queue_t *q, int unique, void *data, queue_node_t **pnp) {
    int err = 0;
    queue_node_t *node = NULL;
    queue_assert_locked(q);
    if (q == NULL)
        return -EINVAL;

    if (unique) {
        if ((err = queue_contains(q, data, NULL)) == 0)
            return -EEXIST;
    }

    if ((node = kmalloc(sizeof(*node))) == NULL)
        return -ENOMEM;

    memset(node, 0, sizeof *node);

    node->data = data;

    if (q->head == NULL) {
        q->tail = node;
    } else {
        q->head->prev = node;
        node->next = q->head;
    }

    q->head = node;
    node->queue = q;
    q->q_count++;

    if (pnp)
        *pnp = node;
    return 0;
}

int dequeue(queue_t *q, void **pdp) {
    queue_node_t *node = NULL, *prev = NULL, *next = NULL;
    queue_assert_locked(q);

    if (q == NULL || pdp == NULL)
        return -EINVAL;

    node = q->head;
    if (node)
    {
        *pdp = node->data;
        prev = node->prev;
        next = node->next;


        if (prev)
            prev->next = next;
        if (next)
            next->prev = prev;
        if (node == q->head)
            q->head = next;
        if (node == q->tail)
            q->tail = prev;

        q->q_count--;
        node->queue = NULL;
        kfree(node);

        return 0;
    }

    return -ENOENT;
}

int queue_remove_node(queue_t *q, queue_node_t *__node)
{
    queue_node_t *next = NULL, *prev = NULL;
    queue_assert_locked(q);
    if (q == NULL || __node == NULL)
        return -EINVAL;

    forlinked(node, q->head, next)
    {
        next = node->next;
        prev = node->prev;
        if (__node == node)
        {
            if (prev)
                prev->next = next;
            if (next)
                next->prev = prev;
            if (node == q->head)
                q->head = next;
            if (node == q->tail)
                q->tail = prev;

            q->q_count--;
            node->queue = NULL;
            kfree(node);
            return 0;
        }
    }

    return -ENOENT;
}

int queue_remove(queue_t *q, void *data)
{
    queue_node_t *next = NULL, *prev = NULL;
    queue_assert_locked(q);

    if (q == NULL)
        return -EINVAL;

    forlinked(node, q->head, next)
    {
        next = node->next;
        prev = node->prev;
        if (node->data == data)
        {
            if (prev)
                prev->next = next;
            if (next)
                next->prev = prev;
            if (node == q->head)
                q->head = next;
            if (node == q->tail)
                q->tail = prev;

            q->q_count--;
            node->queue = NULL;
            kfree(node);
            return 0;
        }
    }

    return -ENOENT;
}

int dequeue_tail(queue_t *q, void **pdp) {
    queue_node_t *next = NULL, *node = NULL, *prev = NULL;

    queue_assert_locked(q);
    if (q == NULL || pdp == NULL)
        return -EINVAL;

    node = q->tail;
    
    if (node) {
        next = node->next;
        *pdp = node->data;
        prev = node->prev;

        if (prev)
            prev->next = next;
        if (next)
            next->prev = prev;
        if (node == q->head)
            q->head = next;
        if (node == q->tail)
            q->tail = prev;

        q->q_count--;
        node->queue = NULL;
        kfree(node);

        return 0;
    }

    return -ENOENT;
}