#pragma once

#include <ds/queue.h>
#include <sync/assert.h>
#include <mm/kalloc.h>

typedef struct stack {
    queue_t     s_queue;
    spinlock_t  s_lock;
} stack_t;

#define stack_assert(s)         ({assert(s, "No stack"); })
#define stack_lock(s)           ({stack_assert(s); spin_lock(&(s)->s_lock); })
#define stack_unlock(s)         ({stack_assert(s); spin_unlock(&(s)->s_lock); })
#define stack_islocked(s)       ({stack_assert(s); spin_islocked(&(s)->s_lock); })
#define stack_assert_locked(s)  ({stack_assert(s); spin_assert_locked(&(s)->s_lock); })

#define STACK_INIT()            ((stack_t){0})
#define STACK_NEW()             (&(stack_t){0})

static inline int stack_init(stack_t *s) {
    if (s == NULL)
        return -EINVAL;
    s->s_queue = QUEUE_INIT();
    s->s_lock = SPINLOCK_INIT();
    return 0;
}

static inline int stack_push(stack_t *s, void *pd) {
    if (s == NULL)
        return -EINVAL;
    
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    if (enqueue(&s->s_queue, pd) == NULL) {
        queue_unlock(&s->s_queue);
        return -ENOMEM;
    }
    queue_unlock(&s->s_queue);

    return 0;   
}

static inline int stack_pop(stack_t *s, void **pdp) {
    int err = 0;

    if (s == NULL)
        return -EINVAL;
    
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    if ((err = dequeue_tail(&s->s_queue, pdp))) {
        queue_unlock(&s->s_queue);
        return err;
    }
    queue_unlock(&s->s_queue);
    return 0; 
}

static inline int stack_remove(stack_t *s, void *data) {
    int err = 0;
    if (s == NULL)
        return -EINVAL;
    stack_assert_locked(s);
    queue_lock(&s->s_queue);
    if ((err = queue_remove(&s->s_queue, data))) {
        queue_unlock(&s->s_queue);
        return err;
    }
    queue_unlock(&s->s_queue);
    return 0;
}

static inline int stack_contains(stack_t *s, void *data) {
    int err = 0;
    if (s == NULL)
        return -EINVAL;
    stack_assert_locked(s);
    queue_lock(&s->s_queue);
    if ((err = queue_contains(&s->s_queue, data, NULL))) {
        queue_unlock(&s->s_queue);
        return err;
    }
    queue_unlock(&s->s_queue);
    return 0;
}

static inline size_t stack_size(stack_t *s) {
    stack_assert_locked(s);
    queue_lock(&s->s_queue);
    size_t size = queue_count(&s->s_queue);
    queue_unlock(&s->s_queue);
    return size;
}

static inline int stack_flush(stack_t *s) {
    stack_assert_locked(s);
    if (s == NULL)
        return -EINVAL;

    queue_lock(&s->s_queue);
    queue_flush(&s->s_queue);
    queue_unlock(&s->s_queue);

    return 0;
}

static inline int stack_alloc(stack_t **psp) {
    int err = 0;
    stack_t *s = NULL;
    if (psp == NULL)
        return -EINVAL;
    
    if ((s = kmalloc(sizeof *s)) == NULL)
        return -ENOMEM;

    if ((err = stack_init(s)))
        goto error;
    *psp = s;
    return 0;
error:
    if (s) kfree((void *)s);
    return err;
}

static inline void stack_free(stack_t *s) {
    if (s == NULL)
        return;
    if (!stack_islocked(s))
        stack_lock(s);
    stack_flush(s);
    stack_unlock(s);
    kfree(s);
}

static inline int stack_peek(stack_t *s, void **pdp) {
    long err = 0;
    if (s == NULL || pdp == NULL)
        return -EINVAL;
    
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    if ((err = queue_count(&s->s_queue)) == 0) {
        queue_unlock(&s->s_queue);
        return -ENONET;
    }
    *pdp = s->s_queue.head->data;
    queue_unlock(&s->s_queue);

    return 0;
}