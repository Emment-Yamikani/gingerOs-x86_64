#include <sys/thread.h>
#include <sys/_signal.h>
#include <sync/cond.h>
#include <sys/sysproc.h>
#include <core/refcnt.h>
#include <mm/kalloc.h>
#include <core/mutex.h>
#include <sys/sleep.h>

#include <ds/bitmap.h>

__unused static MUTEX(m);

__unused static void th(void) {
    // printk("TID(%d) runnning...\n", gettid());

}

static void test(void) {
    bitmap_t *bm = NULL;

    bitmap_alloc(MiB(16) / PGSZ, &bm);

    bitmap_set(bm, 32, 128);

    usize pos;

    bitmap_alloc_range(bm, 1024, &pos);
    bitmap_alloc_range(bm, 32, &pos);
    bitmap_alloc_range(bm, 32, &pos);

    bitmap_unset(bm, 512, 128);
    bitmap_alloc_range(bm, 128, &pos);
    bitmap_dump_range_with_columns(bm, 0, bm->bm_size -1, 3);
    
    
    loop() ;//thread_join(0, NULL, NULL);
    for (int i = 0; i < 150; ++i)
        kthread_create(NULL, (thread_entry_t)th,
            NULL, THREAD_CREATE_SCHED, NULL);

} BUILTIN_THREAD(test, test, NULL);
