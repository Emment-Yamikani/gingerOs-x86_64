#include <bits/errno.h>
#include <ds/btree.h>
#include <ds/queue.h>
#include <ginger/jiffies.h>
#include <lib/printk.h>
#include <sync/cond.h>
#include <sync/spinlock.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/_time.h>

static SPINLOCK(res_lock);
static jiffies_t        jiffies     = 0;
static struct timespec  jiffies_res = {0};
static queue_t          *sleep_queue= QUEUE_NEW();

void jiffies_update(void) {
    atomic_inc(&jiffies);
    sched_wakeall(sleep_queue);
}

jiffies_t jiffies_get(void) {
    return (jiffies_t)atomic_read(&jiffies);
}

void jiffies_timed_wait(double s) {
    jiffies_t jiffy = jiffies_get() + s_TO_jiffies(s);
    while (time_before(jiffies_get(), jiffy));
}

jiffies_t jiffies_sleep(jiffies_t jiffy) {
    jiffies_t now = 0;
    jiffy += jiffies_get();
    while (time_before((now = jiffies_get()), jiffy)) {
        current_lock();
        if ((sched_sleep(sleep_queue, T_ISLEEP, NULL))) {
            current_unlock();
            break;
        }
        current_unlock();
    }
    return jiffy - now;
}

int jiffies_getres(struct timespec *res) {
    if (!res)
        return -EINVAL;
    spin_lock(res_lock);
    *res = jiffies_res;
    spin_unlock(res_lock);
    return 0;
}

int jiffies_gettime(struct timespec *tp __unused) {
    return -ENOSYS;
}

int jiffies_settime(const struct timespec *tp __unused) {
    return -ENOSYS;
}