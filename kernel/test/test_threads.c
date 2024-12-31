#include <sys/thread.h>
#include <sys/_signal.h>
#include <sync/cond.h>
#include <sys/sysproc.h>
#include <core/refcnt.h>
#include <mm/kalloc.h>
#include <core/mutex.h>
#include <sys/sleep.h>
#include <mm/zone.h>
#include <ds/bitmap.h>

__unused static MUTEX(m);

__unused static void th(void) {
    // printk("TID(%d) runnning...\n", gettid());

}

static void test(void) {
    void *p = NULL;
    __page_alloc_n(GFP_KERNEL, 4, &p);
    printk("addr: %p, start: %p\n", p, zones[ZONEi_NORM].start);

    __page_alloc_n(GFP_KERNEL, 4, &p);

    for (int i = 0; i < 128; ++i)
        kthread_create(NULL, (thread_entry_t)th,
            NULL, THREAD_CREATE_SCHED, NULL);

    loop() ;//thread_join(0, NULL, NULL);
} BUILTIN_THREAD(test, test, NULL);
