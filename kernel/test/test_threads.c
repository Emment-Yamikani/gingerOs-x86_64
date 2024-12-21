#include <sys/thread.h>
#include <sys/_signal.h>
#include <sync/cond.h>
#include <sys/sysproc.h>
#include <core/refcnt.h>
#include <mm/kalloc.h>
#include <core/mutex.h>
#include <sys/sleep.h>

__unused static MUTEX(m);

__unused static void th(void) {
    printk("TID(%d) runnning...\n", gettid());
}

static void test(void) {
    for (int i = 0; i < 300; ++i)
        kthread_create(NULL, (thread_entry_t)th,
            NULL, THREAD_CREATE_SCHED, NULL);

    loop() {
        thread_join(0, NULL, NULL);
    }
} BUILTIN_THREAD(test, test, NULL);