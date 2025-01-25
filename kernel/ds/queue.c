#include <bits/errno.h>
#include <lib/string.h>
#include <mm/kalloc.h>
#include <ds/queue.h>
#include <core/debug.h>

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

void queue_flush(queue_t *q) {
    queue_node_t *next = NULL, *prev = NULL;
    queue_assert_locked(q);

    forlinked(node, q->head, next) {
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

void queue_free(queue_t *q) {
    if (!queue_islocked(q))
        queue_lock(q);
    queue_flush(q);
    queue_unlock(q);
    kfree(q);
}

usize queue_count(queue_t *q) {
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

    forlinked(node, q->head, next) {
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
    else {
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
    if (node) {
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

int queue_remove_node(queue_t *q, queue_node_t *__node) {
    queue_node_t *next = NULL, *prev = NULL;
    queue_assert_locked(q);
    if (q == NULL || __node == NULL)
        return -EINVAL;

    forlinked(node, q->head, next) {
        next = node->next;
        prev = node->prev;
        if (__node == node) {
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

int queue_remove(queue_t *q, void *data) {
    queue_node_t *next = NULL, *prev = NULL;
    queue_assert_locked(q);

    if (q == NULL)
        return -EINVAL;

    forlinked(node, q->head, next) {
        next = node->next;
        prev = node->prev;
        if (node->data == data) {
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

int queue_rellocate_node(queue_t *q, queue_node_t *node, queue_relloc_t whence) {
    queue_node_t *next = NULL;
    queue_node_t *prev = NULL;

    if (q == NULL || node == NULL)
        return -EINVAL;

    queue_assert_locked(q);

    if (node->queue != q)
        return -EPERM;
    
    if (whence != QUEUE_RELLOC_HEAD && whence != QUEUE_RELLOC_TAIL)
        return -EINVAL;
    
    prev = node->prev;
    next = node->next;

    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    if (q->head == node)
        q->head = next;
    if (q->tail == node)
        q->tail = prev;

    node->prev = NULL;
    node->next = NULL;

    switch (whence) {
    case QUEUE_RELLOC_HEAD:
        if (q->head == NULL) {
            q->tail = node;
        } else {
            q->head->prev = node;
            node->next = q->head;
        }
        q->head = node;
        break;
    case QUEUE_RELLOC_TAIL:
        if (q->head == NULL) {
            q->head = node;
        } else {
            q->tail->next = node;
            node->prev = q->tail;
        }
        q->tail = node;
        break;
    }

    return 0;
}

int queue_rellocate(queue_t *q, void *data, queue_relloc_t whence) {
    int          err   = 0;
    queue_node_t *node = NULL;
    if (q == NULL)
        return -EINVAL;

    queue_assert_locked(q);
    
    // TODO: may need to keep a queue node ptr to quicken the rellocation
    // as hat will eliminate the need for iteration on the queue searching
    // for the data item.    
    if ((err = queue_contains(q, data, &node)))
        return err;

    return queue_rellocate_node(q, node, whence);
}

/**
 * @brief Migrates a specified number of nodes from a source queue to a destination queue.
 *
 * This function detaches a contiguous range of nodes starting from a given position
 * in the source queue and attaches them to the head or tail of the destination queue,
 * as specified by the `whence` parameter. The function assumes that locks on both
 * the source and destination queues are held prior to calling.
 *
 * @param dstq Pointer to the destination queue where the nodes will be migrated.
 * @param srcq Pointer to the source queue from which the nodes will be migrated.
 * @param start_pos The starting position (0-based index) of the nodes in the source queue
 *                  to begin migration.
 * @param num_nodes The number of nodes to migrate from the source queue.
 * @param whence Specifies where to attach the nodes in the destination queue.
 *               - `QUEUE_RELLOC_HEAD`: Attach nodes to the head of the destination queue.
 *               - `QUEUE_RELLOC_TAIL`: Attach nodes to the tail of the destination queue.
 *
 * @return int
 * - `0`: Success, the nodes were successfully migrated.
 * - `-EINVAL`: Failure, due to one of the following reasons:
 *     - `start_pos` or `num_nodes` is invalid (out of bounds or exceeds the queue size).
 *     - The source queue does not contain sufficient nodes.
 *     - Invalid value provided for `whence`.
 *
 * @note
 * - The function assumes that the caller has already acquired locks on both
 *   the source (`srcq`) and destination (`dstq`) queues before invoking it.
 * - The `queue` field in each migrated node is updated to point to the destination queue.
 *
 * @example
 * Example usage of `queue_node_migrate`:
 *
 * ```c
 * queue_t src_queue = QUEUE_INIT();
 * queue_t dst_queue = QUEUE_INIT();
 *
 * // Populate src_queue with data...
 *
 * queue_lock(&src_queue);
 * queue_lock(&dst_queue);
 *
 * size_t start_pos = 2;     // Start from the 3rd node
 * size_t num_nodes = 3;     // Migrate 3 nodes
 * queue_relloc_t whence = QUEUE_RELLOC_TAIL; // Attach to the tail of dst_queue
 *
 * int result = queue_node_migrate(&dst_queue, &src_queue, start_pos, num_nodes, whence);
 *
 * queue_unlock(&dst_queue);
 * queue_unlock(&src_queue);
 *
 * if (result == 0) {
 *     printf("Nodes migrated successfully.\n");
 * } else {
 *     printf("Failed to migrate nodes.\n");
 * }
 * ```
 */
int queue_node_migrate(queue_t *dstq, queue_t *srcq, usize start_pos, usize num_nodes, queue_relloc_t whence) {
    usize        index       = 0;
    queue_node_t *node       = NULL;
    queue_node_t *last_node  = NULL;
    queue_node_t *first_node = NULL;

    // Ensure both queues are valid and locked
    queue_assert_locked(srcq);
    queue_assert_locked(dstq);

    if ((start_pos >= srcq->q_count) || (num_nodes == 0) || ((start_pos + num_nodes) > srcq->q_count)) {
        return -EINVAL; // Error: invalid position or range
    }

    if (whence != QUEUE_RELLOC_HEAD && whence != QUEUE_RELLOC_TAIL)
        return -EINVAL; // Error: invalid rellocation position

    index   = 0;
    node    = srcq->head;

    // Traverse the source queue to the starting position
    while (node && index < start_pos) {
        node = node->next;
        index++;
    }

    if (!node) {
        return -ENOENT; // Error: starting node not found
    }

    // Extract nodes from the source queue
    first_node = node;
    last_node  = node;

    for (usize i = 1; i < num_nodes; i++) {
        last_node = last_node->next;
    }

    // Update pointers to remove the nodes from the source queue
    if (first_node->prev) {
        first_node->prev->next = last_node->next;
    } else {
        srcq->head = last_node->next; // Update head if removing from the start
    }

    if (last_node->next) {
        last_node->next->prev = first_node->prev;
    } else {
        srcq->tail = first_node->prev; // Update tail if removing from the end
    }

    srcq->q_count -= num_nodes;

    // Detach the nodes to migrate
    first_node->prev = NULL;
    last_node->next  = NULL;

    // Update the node->queue pointer to point to dstq
    forlinked(node, first_node, node->next) {
        node->queue = dstq;
        // debug("node{%p}->{%p}\n", node, node->next);
    }

    // Insert the nodes into the destination queue
    if (whence == QUEUE_RELLOC_HEAD) {
        last_node->next = dstq->head;
        if (dstq->head) {
            dstq->head->prev = last_node;
        }
        dstq->head = first_node;
        if (!dstq->tail) {
            dstq->tail = last_node; // Update tail if the queue was empty
        }
    } else if (whence == QUEUE_RELLOC_TAIL) {
        first_node->prev = dstq->tail;
        if (dstq->tail) {
            dstq->tail->next = first_node;
        }
        dstq->tail = last_node;
        if (!dstq->head) {
            dstq->head = first_node; // Update head if the queue was empty
        }
    }

    dstq->q_count += num_nodes;

    return 0; // Success
}

int queue_move(queue_t *dstq, queue_t *srcq, queue_relloc_t whence) {
    // Ensure both queues are valid and locked
    queue_assert_locked(srcq);
    queue_assert_locked(dstq);

    if (whence != QUEUE_RELLOC_HEAD && whence != QUEUE_RELLOC_TAIL)
        return -EINVAL; // Error: invalid rellocation position
    return queue_node_migrate(dstq, srcq, 0, srcq->q_count, whence);
}

/**
 * @brief Replaces a data entry in the queue.
 *
 * This function traverses the queue to find a node containing `data0` and replaces
 * it with `data1`. The caller must ensure that the queue lock is held before calling
 * this function.
 *
 * @param queue The queue to search in.
 * @param data0 The data to search for and replace.
 * @param data1 The new data to replace `data0` with.
 *
 * @return int 0 on success, -ENOENT if `data0` is not found.
 *
 * @note The queue lock must be held before calling this function.
 * @note data0 and data1 cannot be the same.
 */
int queue_replace(queue_t *queue, void *data0, void *data1) {
    if (!queue || data0 == data1) {
        return -EINVAL; // Invalid arguments.
    }

    queue_assert_locked(queue);

    // Iterate through the queue to find the node containing `data0`.
    forlinked(node, queue->head, node->next) {
        if (node->data == data0) {
            node->data = data1;
            return 0;
        }
    }

    // If we reach here, `data0` was not found.
    return -ENOENT; // No such entry.
}
