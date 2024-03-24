#include <sys/thread.h>
#include <sys/sleep.h>

atomic_t seconds = 0;

void some_thread(void) {
    printk(
        "tid: %d: tgid: %d\n",
        thread_self(),
        current->t_tgid
    );
}

void other_thread(void) {
    printk(
        "tid: %d: tgid: %d\n",
        thread_self(),
        current->t_tgid
    );
}

void new_group(void) {
    kthread_create(
        NULL,
        (thread_entry_t)other_thread,
        NULL,
        THREAD_CREATE_SCHED,
        NULL
    );

    kthread_create(
        NULL,
        (thread_entry_t)some_thread,
        NULL,
        THREAD_CREATE_SCHED,
        NULL
    );
}




















static void *test_spawner(void *arg) {
    kthread_create(
        NULL, (thread_entry_t)new_group,
        NULL, THREAD_CREATE_GROUP |
        THREAD_CREATE_SCHED, NULL
    );

    debugloc();
    return arg;
}
BUILTIN_THREAD(test_spawner, test_spawner, NULL);