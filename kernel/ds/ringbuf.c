#include <ds/ringbuf.h>
#include <bits/errno.h>
#include <mm/kalloc.h>

int ringbuf_new(size_t size, ringbuf_t **rref)
{
    char *buf = NULL;
    assert(rref, "no reference");

    struct ringbuf *ring = kmalloc(sizeof(struct ringbuf));
    if (!ring) return -ENOMEM;

    if (!(buf = kmalloc(size)))
    {
        kfree(ring);
        return -ENOMEM;
    }

    *ring = (struct ringbuf){
        .buf = buf,
        .size = size,
        .head = 0,
        .tail = 0,
        .lock = SPINLOCK_INIT(),
    };

    *rref = ring;
    return 0;
}

void ringbuf_free(struct ringbuf *r)
{
    assert(r, "no ringbuff");
    kfree(r->buf);
    kfree(r);
}

int ringbuf_isempty(ringbuf_t *ring)
{
    ringbuf_assert(ring);
    ringbuf_assert_locked(ring);
    return ring->count == 0;
}

int ringbuf_isfull(ringbuf_t *ring)
{
    ringbuf_assert(ring);
    ringbuf_assert_locked(ring);
    return ring->count == ring->size;
}

size_t ringbuf_read(struct ringbuf *ring, size_t n, char *buf)
{
    size_t size = n;
    ringbuf_assert(ring);
    ringbuf_assert_locked(ring);
    while (n)
    {
        if (ringbuf_isempty(ring))
            break;
        *buf++ = ring->buf[RINGBUF_INDEX(ring, ring->head++)];
        ring->count--;
        n--;
    }
    return size - n;
}

size_t ringbuf_write(struct ringbuf *ring, size_t n, char *buf)
{
    size_t size = n;
    ringbuf_assert(ring);
    ringbuf_assert_locked(ring);    
    while (n)
    {
        if (ringbuf_isfull(ring))
            break;

        ring->buf[RINGBUF_INDEX(ring, ring->tail++)] = *buf++;
        ring->count++;
        n--;
    }
    return size - n;
}

size_t ringbuf_available(struct ringbuf *ring)
{
    ringbuf_assert(ring);
    ringbuf_assert_locked(ring);
    if (ring->tail >= ring->head)
        return ring->tail - ring->head;
    size_t aval = ring->tail + ring->size - ring->head;
    return aval;
}

void ringbuf_debug(ringbuf_t *ring)
{
    ringbuf_assert(ring);
    ringbuf_lock(ring);
    printk("size: %5d\nhead: %5d\ntail: %5d\ncount: %d\n",
        ring->size, RINGBUF_INDEX(ring, ring->head), RINGBUF_INDEX(ring, ring->tail), ring->count);
    ringbuf_unlock(ring);
}