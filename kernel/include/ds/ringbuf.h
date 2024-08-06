#pragma once 

#include <sync/spinlock.h>

#define RINGBUF_INDEX(ring, i) ((i) % ((ring)->size))

typedef struct ringbuf {
    char *buf;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
    spinlock_t lock;
}ringbuf_t;

#define RINGBUF_NEW(nam, sz) (&(ringbuf_t){.buf = (char[sz]){0}, .size = sz, .head = 0, .tail = 0, .lock = SPINLOCK_INIT()})

#define ringbuf_assert(r)           ({ assert(r, "no ringbuf"); })
#define ringbuf_lock(r)             ({ ringbuf_assert(r); spin_lock(&(r)->lock); })
#define ringbuf_unlock(r)           ({ ringbuf_assert(r); spin_unlock(&(r)->lock); })
#define ringbuf_assert_locked(r)    ({ ringbuf_assert(r); spin_assert_locked(&(r)->lock); })

void ringbuf_debug(ringbuf_t *ring);

int ringbuf_new(size_t size, ringbuf_t **);
int ringbuf_init(isize size, ringbuf_t *ring);

void ringbuf_free(ringbuf_t *ring);
int ringbuf_isfull(ringbuf_t *ring);
int ringbuf_isempty(ringbuf_t *ring);
size_t ringbuf_available(ringbuf_t *ring);

size_t ringbuf_read(ringbuf_t *ring, char *buf, size_t n);
size_t ringbuf_write(ringbuf_t *ring, char *buf, size_t n);