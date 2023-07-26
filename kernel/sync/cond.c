#include <lib/printk.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <sync/cond.h>
#include <mm/kalloc.h>
#include <sys/thread.h>
#include <sys/sched.h>

int cond_new(const char *name, cond_t **ref) {
    return cond_init(NULL, name, ref);
}

void cond_free(cond_t *c)
{
    if (c->waiters)
        queue_free(c->waiters);
    if (c->name)
        kfree(c->name);
    kfree(c);
}

int cond_init(cond_t *c, const char *__name, cond_t **ref)
{
    int err = 0;
    int alloc = !c;
    char *name = NULL;
    queue_t *waiters = NULL;

    if ((!c && !ref) || !__name)
        return -EINVAL;
    if (alloc)
    {
        if (!(c = (cond_t *)kmalloc(sizeof *c)))
        {
            err = -ENOMEM;
            goto error;
        }
    }
    if (!(name = combine_strings(__name, "-condition")))
    {
        err = -EINVAL;
        goto error;
    }
    if ((err = queue_new(__name, &waiters)))
        goto error;

    *c = (cond_t){0};

    c->name = name;
    c->waiters = waiters;
    c->guard = SPINLOCK_INIT();

    if (ref)
        *ref = c;
    return 0;
error:
    if (c && alloc)
        kfree(c);
    if (name)
        kfree(name);
    printk("cond_init(): called @ 0x%p, error=%d\n", __retaddr(0), err);
    return err;
}

int cond_wait(cond_t *cond)
{
    int retval = 0;
    assert(cond, "no condition-variable");
    current_assert();

    spin_lock(&cond->guard);
    if ((int)atomic_inc(&cond->count) >= 0)
    {
        current_lock();
        retval = sched_sleep(cond->waiters, &cond->guard);
        current_unlock();
    }
    spin_unlock(&cond->guard);
    return retval;
}

static void cond_wake1(cond_t *cond)
{
    sched_wake1(cond->waiters);
}

void cond_signal(cond_t *cond)
{
    assert(cond, "no condition-variable");
    spin_lock(&cond->guard);
    cond_wake1(cond);
    atomic_dec(&cond->count);
    spin_unlock(&cond->guard);
}

static void cond_wakeall(cond_t *cond)
{
    int waiters = sched_wakeall(cond->waiters);
    if (waiters == 0)
        atomic_write(&cond->count, -1);
    else
        atomic_write(&cond->count, 0);
}

void cond_broadcast(cond_t *cond)
{
    assert(cond, "no condition-variable");
    spin_lock(&cond->guard);
    cond_wakeall(cond);
    spin_unlock(&cond->guard);
}