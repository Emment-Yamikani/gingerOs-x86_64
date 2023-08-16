#include <lib/types.h>
#include <sys/thread.h>
#include <sys/sleep.h>

void start_others(void);

void secondary_thread(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
    
    printk("tid: %d, tgroup: %d\n", thread_self(), current_tgroup()->tg_tgid);

    start_others();
    loop() {
    };
}


void core_start(void) {
    thread_t *t = NULL;
    thread_create(&t, NULL, (thread_entry_t)secondary_thread, NULL);

    thread_unlock(t);
    
    pthread_kill(thread_gettid(t), SIGQUIT);
}

void v(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
    printk("tid: %d, tgroup: %d\n", thread_self(), current_tgroup()->tg_tgid);
    return;
}

void start_others(void) {
    thread_attr_t at = (thread_attr_t){
        .guardsz = 0,
        .stackaddr = 0,
        .detachstate = 1,
        .stacksz = STACKSZMIN,
    };
    thread_create(NULL, &at, (thread_entry_t)v, NULL);
    at.stackaddr = 0;
    thread_create(NULL, &at, (thread_entry_t)v, NULL);
}