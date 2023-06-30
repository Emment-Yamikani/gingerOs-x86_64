#pragma once

#include <ds/queue.h>
#include <sync/spinlock.h>

typedef struct cond
{
    char *name;
    atomic_t count;
    queue_t *waiters;
    spinlock_t guard;
} cond_t;

int cond_wait(cond_t *cond);
void cond_free(cond_t *cond);
void cond_signal(cond_t *cond);
void cond_broadcast(cond_t *cond);
int cond_new(const char *__name, cond_t **ref);
int cond_init(cond_t *c, const char *name, cond_t **ref);