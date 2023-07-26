#include <lib/printk.h>
#include <ds/queue.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <lib/string.h>

void queue_free(queue_t *q)
{
    queue_lock(q);
    queue_flush(q);
    queue_unlock(q);
    if (q->name && (q->flags & 1))
        kfree(q->name);
    if (q->flags & 1) kfree(q);
}

void queue_flush(queue_t *q)
{
    queue_assert_locked(q);
    if (spin_trylock(&q->lock))
        panic("caller not holding spinlock\n", q->name);
    while (dequeue(q))
        ;
}

void *dequeue(queue_t *q)
{
    void *data = NULL;
    queue_node_t *node = NULL;

    queue_assert_locked(q);
    if (spin_trylock(&q->lock))
        panic("caller not holding %s spinlock\n", q->name);

    if (!(node = q->head))
        return NULL;
    if (q->head == q->tail)
        q->head = q->tail = NULL;
    if ((q->head = node->next))
        q->head->prev = NULL;
    data = node->data;
    q->count--;

    kfree(node);
    return data;
}

void *queue_get(queue_t *q)
{
    void *data = NULL;
    queue_lock(q);
    data = dequeue(q);
    queue_unlock(q);
    return data;
}

queue_node_t *enqueue(queue_t *q, void *data)
{
    queue_node_t *node = NULL;

    queue_assert_locked(q);
    if (spin_trylock(&q->lock))
        panic("caller not holding %s spinlock\n", q->name);

    if (!(node = kmalloc(sizeof *node)))
        return NULL;
    *node = (queue_node_t){0};

    if (!q->head)
        q->head = node;

    if (q->tail)
    {
        q->tail->next = node;
        node->prev = q->tail;
    }
    q->count++;
    q->tail = node;
    node->queue = q;
    node->data = data;
    return node;
}

size_t queue_count(queue_t *q)
{
    queue_assert_locked(q);
    if (spin_trylock(&q->lock))
        panic("caller not holding %s spinlock\n", q->name);
    return q->count;
}

queue_node_t *queue_contains(queue_t *q, void *data)
{
    queue_assert_locked(q);
    if (spin_trylock(&q->lock))
        panic("caller not holding %s spinlock\n", q->name);
    forlinked(node, q->head, node->next) if (node->data == data) return node;
    return NULL;
}

int queue_contains_node(queue_t *q, queue_node_t *node)
{
    queue_assert_locked(q);
    if (spin_trylock(&q->lock))
        panic("caller not holding %s spinlock\n", q->name);
    forlinked(n, q->head, n->next) if (n == node) return 1;
    return 0;
}

int queue_new(const char *__name, queue_t **ref)
{
    int err = 0;
    queue_t *q = NULL;
    char *name = NULL;
    
    if (!__name || !ref)
        return -EINVAL;

    if (!(q = kmalloc(sizeof *q)))
    {
        err = -ENOMEM;
        goto error;
    }

    if (!(name = combine_strings(__name, "-queue")))
    {
        err = -ENOMEM;
        goto error;
    }

    *q = (queue_t){0};
        
    q->flags = 1;
    q->name = name;
    q->lock = SPINLOCK_INIT();

    *ref = q;
    return 0;

error:
    if (name)
        kfree(name);
    if (q)
        kfree(q);
    printk("queue_new(): error = %d\n", err);
    return err;
}

int queue_remove_node(queue_t *q, queue_node_t *node)
{
    queue_assert_locked(q);
    assert(node, "no node");

    if (spin_trylock(&q->lock))
        panic("caller not holding %s spinlock\n", q->name);

    if (!queue_contains_node(q, node))
        return -ENOENT;

    if (node->prev)
        node->prev->next = node->next;
    else
        q->head = node->next;
    
    if (node->next)
        node->next->prev = node->prev;
    else
        q->tail = node->prev;

    node->next = NULL;
    node->prev = NULL;

    q->count--;
    kfree(node);

    return 0;
}

int queue_remove(queue_t *q, void *data)
{
    queue_assert_locked(q);
    if (spin_trylock(&q->lock))
        panic("caller not holding %s spinlock\n", q->name);
    queue_node_t *node = queue_contains(q, data);
    if (!node) return -ENOENT;
    return queue_remove_node(q, node);
}