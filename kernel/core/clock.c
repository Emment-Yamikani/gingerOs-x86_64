#include <bits/errno.h>
#include <dev/clocks.h>
#include <sync/atomic.h>
#include <mm/kalloc.h>
#include <lib/types.h>
#include <lib/string.h>
#include <ds/queue.h>
#include <ginger/jiffies.h>

static QUEUE(clk_queue);
static atomic_t nclks = 0;
static atomic_t clkid = 0;

void clock_trigger(void) {
    timeval_t tv;    
    JIFFIES_TO_TIMEVAL(1, &tv);

    queue_lock(clk_queue);
    queue_foreach(clk_t *, clk, clk_queue) {
        clk_lock(clk);
        if (clk->clk_flags & CLK_ARMED) {
            clk->clk_tv = TIMEVAL_SUB(clk->clk_tv, tv);
            if (clk->clk_tv.tv_sec < 0) {
                if (clk->clk_flags & CLK_RESET)
                    clk->clk_tv = clk->clk_default_tv;
                else{
                    clk->clk_flags &= ~CLK_ARMED;
                    clk->clk_tv = (timeval_t) {0};
                }
                clk->clk_entry(clk->clk_arg);
            } else if (TIMEVAL_EQ(&clk->clk_tv, &((timeval_t){0}))) {
                if (clk->clk_flags & CLK_RESET)
                    clk->clk_tv = clk->clk_default_tv;
                else{
                    clk->clk_flags &= ~CLK_ARMED;
                    clk->clk_tv = (timeval_t) {0};
                }
                clk->clk_entry(clk->clk_arg);
            }
        }
        clk_unlock(clk);
    }
    queue_unlock(clk_queue);
}

int clock_set(timeval_t *ts, void (*entry)(), void *arg, int flags, clockid_t *ref) {
    int         err = 0;
    clockid_t   id  = 0;
    clk_t       *clk= NULL;
    
    if (atomic_read(&nclks) >= NCLK)
        return -EAGAIN;
    
    if ((ts == NULL) || ((ts->tv_usec == 0) && (ts->tv_sec == 0)))
        return -EINVAL;

    if (entry == NULL)
        return -EFAULT;

    if (ref == NULL)
        return -EINVAL;    

    if (NULL == (clk = kcalloc(1, sizeof *clk)))
        return -ENOMEM;
    
    clk->clk_tv         = *ts;
    clk->clk_default_tv = *ts;
    clk->clk_arg        = arg;
    clk->clk_entry      = entry;
    clk->clk_flags      = flags | CLK_ARMED;
    clk->clk_spinlock   = SPINLOCK_INIT();
    clk->clk_id         = id = (clockid_t) atomic_inc(&clkid);

    queue_lock(clk_queue);
    err = enqueue(clk_queue, (void *)clk, 1, NULL);
    queue_unlock(clk_queue);

    if (err) goto error;

    *ref = id;
    return 0;
error:
    kfree(clk);
    printk("%s:%d: failed to set clock, error: %d\n", __FILE__, __LINE__, err);
    return err;
}