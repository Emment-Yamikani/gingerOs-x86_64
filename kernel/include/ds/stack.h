#pragma once

#include <ds/queue.h>

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

int stack_init(stack_t *s);
int stack_push(stack_t *s, void *pd);
int stack_pop(stack_t *s, void **pdp);
int stack_remove(stack_t *s, void *data);
int stack_contains(stack_t *s, void *data);
size_t stack_size(stack_t *s);
int stack_flush(stack_t *s);
int stack_alloc(stack_t **psp);
void stack_free(stack_t *s);
int stack_peek(stack_t *s, int top, void **pdp);