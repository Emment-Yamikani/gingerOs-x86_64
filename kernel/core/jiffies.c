#include <bits/errno.h>
#include <ds/btree.h>
#include <ds/queue.h>
#include <lib/printk.h>
#include <sync/cond.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <sys/_time.h>
#include <sync/spinlock.h>
#include <ginger/jiffies.h>

static struct timespec      jiffies_res = {0};
static SPINLOCK(jiffies_res_lock);

static jiffies_t            jiffies = 0;
// static SPINLOCK(jiffies_lock);
static queue_t              *jiffies_sleep_queue = QUEUE_NEW(/*"jiffies-sleep-queue"*/);

void jiffies_update(void) {
    // spin_lock(jiffies_lock);
    atomic_inc(&jiffies);
    // spin_unlock(jiffies_lock);
    sched_wakeall(jiffies_sleep_queue);
}

jiffies_t jiffies_get(void) {
    // spin_lock(jiffies_lock);
    jiffies_t jiffy = atomic_read(&jiffies);
    // spin_unlock(jiffies_lock);
    return jiffy;
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
        if ((sched_sleep(jiffies_sleep_queue, T_ISLEEP, NULL))) {
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

    spin_lock(jiffies_res_lock);
    *res = jiffies_res;
    spin_unlock(jiffies_res_lock);

    return 0;
}

int jiffies_gettime(struct timespec *tp __unused) {
    return 0;
}

int jiffies_settime(const struct timespec *tp __unused) {
    return 0;
}