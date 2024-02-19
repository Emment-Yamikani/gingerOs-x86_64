#pragma once

#include <sync/spinlock.h>

struct queue;

typedef struct queue_node {
    struct queue_node   *prev;
    void                *data;
    struct queue_node   *next;
    struct queue        *queue;
} queue_node_t;

typedef struct queue {
    queue_node_t    *head;
    queue_node_t    *tail;
    size_t          q_count;
    spinlock_t      q_lock;
} queue_t;

#define queue_assert(q)         ({ assert((q), "No queue"); })
#define queue_lock(q)           ({ queue_assert(q); spin_lock(&(q)->q_lock); })
#define queue_unlock(q)         ({ queue_assert(q); spin_unlock(&(q)->q_lock); })
#define queue_islocked(q)       ({ queue_assert(q); spin_islocked(&(q)->q_lock); })
#define queue_assert_locked(q)  ({ queue_assert(q); spin_assert_locked(&(q)->q_lock); })

#define QUEUE_INIT() ((queue_t){0})
#define QUEUE_NEW() (&QUEUE_INIT())

#define INIT_QUEUE(q) ({           \
    queue_assert(q);               \
    memset(q, 0, sizeof *(q));     \
    (q)->q_lock = SPINLOCK_INIT(); \
})

#define queue_foreach(type, item, queue)                                          \
    queue_assert_locked(queue);                                                   \
    for (type item = (queue) ? (queue)->head ? (queue)->head->data : NULL : NULL, \
              *node = (typeof(node))((queue) ? (queue)->head : NULL);             \
         node != NULL; node = (typeof(node))((queue_node_t *)node)->next,         \
              item = (type)(node ? ((queue_node_t *)node)->data : NULL))

int queue_alloc(queue_t **pqp);

void queue_flush(queue_t *q);

void queue_free(queue_t *q);

size_t queue_count(queue_t *q);

int queue_peek(queue_t *q, int tail, void **pdp);

int queue_contains(queue_t *q, void *data, queue_node_t **pnp);

int enqueue(queue_t *q, void *data, int unique, queue_node_t **pnp);

int enqueue_head(queue_t *q, int unique, void *data, queue_node_t **pnp);

int dequeue(queue_t *q, void **pdp);

int queue_remove_node(queue_t *q, queue_node_t *__node);

int queue_remove(queue_t *q, void *data);

int dequeue_tail(queue_t *q, void **pdp);