#include <bits/errno.h>
#include <ds/stack.h>
#include <mm/kalloc.h>
#include <sync/assert.h>

int stack_init(stack_t *s) {
    if (s == NULL)
        return -EINVAL;

    s->s_queue = QUEUE_INIT();
    s->s_lock = SPINLOCK_INIT();
    return 0;
}

int stack_push(stack_t *s, void *pd) {
    int err = 0;
    
    if (s == NULL)
        return -EINVAL;
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    err = enqueue(&s->s_queue, pd, 0, NULL);
    queue_unlock(&s->s_queue);
    return err;
}

int stack_pop(stack_t *s, void **pdp) {
    int err = 0;

    if (s == NULL)
        return -EINVAL;
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    err = dequeue_tail(&s->s_queue, pdp);
    queue_unlock(&s->s_queue);
    return err; 
}

int stack_remove(stack_t *s, void *data) {
    int err = 0;
    if (s == NULL)
        return -EINVAL;
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    err = queue_remove(&s->s_queue, data);
    queue_unlock(&s->s_queue);
    return err;
}

int stack_contains(stack_t *s, void *data) {
    int err = 0;
    if (s == NULL)
        return -EINVAL;
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    err = queue_contains(&s->s_queue, data, NULL);
    queue_unlock(&s->s_queue);
    return err;
}

size_t stack_size(stack_t *s) {
    size_t size = 0;
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    size = queue_count(&s->s_queue);
    queue_unlock(&s->s_queue);
    return size;
}

int stack_flush(stack_t *s) {
    if (s == NULL)
        return -EINVAL;
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    queue_flush(&s->s_queue);
    queue_unlock(&s->s_queue);
    return 0;
}

int stack_alloc(stack_t **psp) {
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

void stack_free(stack_t *s) {
    if (s == NULL)
        return;
    if (!stack_islocked(s))
        stack_lock(s);

    stack_flush(s);
    stack_unlock(s);
    kfree(s);
}

int stack_peek(stack_t *s, int top, void **pdp) {
    int err = 0;
    if (s == NULL || pdp == NULL)
        return -EINVAL;
    stack_assert_locked(s);

    queue_lock(&s->s_queue);
    err = queue_peek(&s->s_queue, top, pdp);
    queue_unlock(&s->s_queue);
    return err;
}