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
    printk("thread(%p)\n", current);
    thread_exit(0);
}

static void test_grp(void) {
    int err = 0;
    thread_info_t info;

    for (int i = 0; i < 5; ++i)
        kthread_create(NULL, (thread_entry_t)th,
            NULL, THREAD_CREATE_SCHED, NULL);

    loop() {
        if ((err = thread_join(0, &info, NULL)) == 0)
            printk("tid: %d exited.\n", info.ti_tid);
    }
}

static void test(void) {
    printk("\n");
    BUILTIN_THREAD_ANOUNCE(__func__);
    
    kthread_create(NULL, (thread_entry_t)test_grp, NULL,
        THREAD_CREATE_GROUP | THREAD_CREATE_SCHED, NULL);
} BUILTIN_THREAD(test, test, NULL);