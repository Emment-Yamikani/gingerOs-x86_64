#pragma once

#include <ds/queue.h>
#include <sync/spinlock.h>

typedef struct cond {
    atomic_t   count;
    queue_t    waiters;
    spinlock_t guard;
} cond_t;

#define COND_INIT()     ((cond_t){0})
#define COND_NEW()      (&COND_INIT())

int cond_new(cond_t **ref);
int cond_wait(cond_t *cond);
void cond_free(cond_t *cond);
void cond_signal(cond_t *cond);
void cond_broadcast(cond_t *cond);
int cond_init(cond_t *c, cond_t **ref);