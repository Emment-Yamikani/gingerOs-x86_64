#include <lib/printk.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <sync/cond.h>
#include <mm/kalloc.h>
#include <sys/thread.h>
#include <sys/sched.h>

int cond_new(cond_t **ref) {
    return cond_init(NULL, ref);
}

void cond_free(cond_t *c) {
    queue_lock(&c->waiters);
    queue_flush(&c->waiters);
    kfree(c);
}

int cond_init(cond_t *c, cond_t **ref) {
    int err   = 0;
    int alloc = !c;

    if ((!c && !ref))
        return -EINVAL;

    if (alloc) {
        if (!(c = (cond_t *)kmalloc(sizeof *c))) {
            err = -ENOMEM;
            goto error;
        }
    }

    *c = (cond_t){0};

    c->waiters  = QUEUE_INIT();
    c->guard    = SPINLOCK_INIT();

    if (ref)
        *ref = c;
    return 0;
error:
    if (c && alloc)
        kfree(c);
    printk("cond_init(): called @ 0x%p, error=%d\n", __retaddr(0), err);
    return err;
}

int cond_wait(cond_t *cond) {
    int retval = 0;

    assert(cond, "no condition-variable");
    current_assert();

    spin_lock(&cond->guard);
    if ((int)atomic_inc(&cond->count) >= 0) {
        current_lock();
        retval = sched_sleep(&cond->waiters, T_ISLEEP, &cond->guard);
        current_unlock();
    }
    spin_unlock(&cond->guard);
    return retval;
}

int cond_wait_releasing(cond_t *cond, spinlock_t *lk) {
    int retval = 0;

    if (lk)
        spin_unlock(lk);
    retval = cond_wait(cond);
    if (lk)
        spin_lock(lk);
    return retval;
}

static void cond_wake1(cond_t *cond) {
    sched_wake1(&cond->waiters);
}

void cond_signal(cond_t *cond) {
    assert(cond, "no condition-variable");
    spin_lock(&cond->guard);
    cond_wake1(cond);
    atomic_dec(&cond->count);
    spin_unlock(&cond->guard);
}

static void cond_wakeall(cond_t *cond) {
    int waiters = sched_wakeall(&cond->waiters);
    if (waiters == 0)
        atomic_write(&cond->count, -1);
    else
        atomic_write(&cond->count, 0);
}

void cond_broadcast(cond_t *cond) {
    assert(cond, "no condition-variable");
    spin_lock(&cond->guard);
    cond_wakeall(cond);
    spin_unlock(&cond->guard);
}