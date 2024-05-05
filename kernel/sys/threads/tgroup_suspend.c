#include <sys/thread.h>

int tgroup_suspend(queue_t *tgroup) {
    tgroup_lock(tgroup);
    queue_foreach(thread_t *, thread, tgroup) {
        if (current == thread)
            continue;
        thread_lock(thread);
        thread_setflags(thread, THREAD_SUSPEND);
        thread_unlock(thread);
    }
    tgroup_unlock(tgroup);
    return 0;
}