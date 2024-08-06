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

// initialize a queue struct to 'all-zeros'.
#define QUEUE_INIT()    ((queue_t){0})

// get a preallocated queue pointer at compile time.
#define QUEUE_NEW()     (&QUEUE_INIT())

/**
 * @brief shorthand for declaring a queue
 * this uses QUEUE_NEW() to provided the initialize struct
 * returning a pointer to it.
 * @param name is the name of the queue pointer.
 */
#define QUEUE(name)     queue_t *name = QUEUE_NEW()

#define queue_assert(q)         ({ assert((q), "No queue"); })
#define queue_lock(q)           ({ queue_assert(q); spin_lock(&(q)->q_lock); })
#define queue_unlock(q)         ({ queue_assert(q); spin_unlock(&(q)->q_lock); })
#define queue_islocked(q)       ({ queue_assert(q); spin_islocked(&(q)->q_lock); })
#define queue_assert_locked(q)  ({ queue_assert(q); spin_assert_locked(&(q)->q_lock); })

// initialize a queue at runtime with this macro.
#define INIT_QUEUE(q) ({           \
    queue_assert(q);               \
    memset(q, 0, sizeof *(q));     \
    (q)->q_lock = SPINLOCK_INIT(); \
})

/**
 * @brief Iterates over a queue's node.
 * caller must have held queue->q_lock prior to calling this macro.
 */
#define queue_foreach(type, item, queue)                                          \
    queue_assert_locked(queue);                                                   \
    for (type item = (queue) ? (queue)->head ? (queue)->head->data : NULL : NULL, \
              *node = (typeof(node))((queue) ? (queue)->head : NULL);             \
         node != NULL; node = (typeof(node))((queue_node_t *)node)->next,         \
              item = (type)(node ? ((queue_node_t *)node)->data : NULL))

// @brief free memory allocated via queue_alloc()
void queue_free(queue_t *q);

// flushes all data currently on the queue specified by parameter 'q'
void queue_flush(queue_t *q);

// allocates a queue queue returning a pointer to the newly allocated queue.
int queue_alloc(queue_t **pqp);

// returns the number of items currently on the queue specified by param 'q'.
size_t queue_count(queue_t *q);

/**
 * @brief Used to take a peek at the front
 * or back-end of the queue specified by q.
 *
 * @param q the queue to be peeked
 * @param tail if non-zero, peeks the data at the tail-end,
 *  otherwise the front-end data is peeked.
 * @param pdp pointer to a location to store a pointer to the peeked data.
 * @return int 0 on success, otherwise reports the reports the error that has occured.
 */
int queue_peek(queue_t *q, int tail, void **pdp);

/**
 * @brief checks the availability of the data specified by data.
 * 
 * @param q queue being queried for the data.
 * @param data the data to be queried.
 * @param pnp pointer in which a pointer to node containing
 * the data.
 * @return int 0 on success otherwise an error code is returned.
 */
int queue_contains(queue_t *q, void *data, queue_node_t **pnp);


/**
 * @brief Dequeue a data item from the queue.
 *  data once is removed from the queue once dequeued
 * @param q queue from which data is retrieved.
 * @param pdp pointer in which to return the data.
 * @return int 0 on success, otherwise error code is returned.
 */
int dequeue(queue_t *q, void **pdp);

/**
 * @brief same as dequeue(), except the retrieval happens at the tail-end.
 *
 * @param q queue from which data is to be retrieved.
 * @param pdp pointer in which to return the data.
 * @return int 0 on success, otherwise error code is returned.
 */
int dequeue_tail(queue_t *q, void **pdp);

/**
 * @brief enqueue a data item onto the queue.
 * 
 * @param q queue on which the datum is enqueued.
 * @param data data to be enqueued.
 * @param unique if non-zero, enqueue() will deny multiple data items
 * that is similar.
 * @param pnp if non-null, returned pointer to the node holding this data.
 * @return int 0 on success or error code otherwise.
 */
int enqueue(queue_t *q, void *data, int unique, queue_node_t **pnp);

/**
 * @brief Same as enqueue(), except the data is enqueued
 * at the front-end of the queue.
 *
 * @param q queue on which the datum is enqueued.
 * @param data data to be enqueued.
 * @param unique if non-zero, enqueue() will deny multiple data items
 * that is similar.
 * @param pnp if non-null, returned pointer to the node holding this data.
 * @return int 0 on success or error code otherwise.
 */
int enqueue_head(queue_t *q, int unique, void *data, queue_node_t **pnp);

/**
 * @brief Removes a data item from the queue
 * 
 * @param q queue from which the data item is removed.
 * @param data the data to be removed.
 * @return int 0 on success, otherwise and error code is returned.
 */
int queue_remove(queue_t *q, void *data);

/**
 * @brief Removes a data node from the queue
 *
 * @param q queue from which the node item is removed.
 * @param data the node to be removed.
 * @return int 0 on success, otherwise and error code is returned.
 */
int queue_remove_node(queue_t *q, queue_node_t *__node);

// rellocation points, used with queue rellocation functions.
typedef enum {
    QUEUE_RELLOC_TAIL,  // rellocate to the tail-end.
    QUEUE_RELLOC_HEAD   // rellocate to the front-end.
} queue_relloc_t;

/**
 * @brief Rellocates  a data item to the fron or back depending
 * on the value of 'head'.
 *
 * @param q queue on which to apply the operation.
 * @param node contains the data to be rellocated.
 * @param whence specifies where the data is the be rellocated to.
 *  see above enum queue_relloc_t typedef.
 * @return int 0 on success, otherwise and error code is returned.
 */
int queue_rellocate_node(queue_t *q, queue_node_t *node, queue_relloc_t whence);

// same as above only difference is this take a data pointer not a node.
int queue_rellocate(queue_t *q, void *data, queue_relloc_t whence);