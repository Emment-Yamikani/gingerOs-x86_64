#include <lib/types.h>
#include <sys/thread.h>

void start_others(void);

void secondary_thread(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
    
    printk("tid: %d, tgroup: %d\n", thread_self(), current_tgroup()->tg_tgid);

    start_others();
    loop();
}


void core_start(void) {
    thread_create(NULL, NULL, (thread_entry_t)secondary_thread, NULL);
}

void v(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
    printk("tid: %d, tgroup: %d\n", thread_self(), current_tgroup()->tg_tgid);
    return;
}

void start_others(void) {
    thread_create(NULL, NULL, (thread_entry_t)v, NULL);
    thread_create(NULL, NULL, (thread_entry_t)v, NULL);
    thread_create(NULL, NULL, (thread_entry_t)v, NULL);
    thread_create(NULL, NULL, (thread_entry_t)v, NULL);
}