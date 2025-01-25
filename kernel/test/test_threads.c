#include <sys/thread.h>
#include <sys/_signal.h>
#include <sync/cond.h>
#include <sys/sysproc.h>
#include <core/refcnt.h>
#include <mm/kalloc.h>
#include <core/mutex.h>
#include <sys/sleep.h>
#include <mm/zone.h>
#include <core/debug.h>
#include <ds/bitmap.h>


extern void _sim_trap(int x);
void th() {
    // BUILTIN_THREAD_ANOUNCE(__func__);
    loop() debug("cpu[%d], thread[%d:%d]\n", getcpuid(), getpid(), gettid());
}

void test(void) {
    for (int i = 0; i < 300; ++i) {
        kthread_create(NULL, (thread_entry_t)th,
            NULL, THREAD_CREATE_SCHED, NULL);
    }

    loop();
} BUILTIN_THREAD(test, test, NULL);