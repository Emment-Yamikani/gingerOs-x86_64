#include <sys/thread.h>
#include <ginger/jiffies.h>

__noreturn void icache_sync(void) {
    BUILTIN_THREAD_ANOUNCE("icache_sync");
    loop() {
        jiffies_sleep(5);    
    }
}

// BUILTIN_THREAD(icache_sync, icache_sync, NULL);


__noreturn void icache_prefetch(void) {
    BUILTIN_THREAD_ANOUNCE("PREFETCH thread");
    loop() {
        jiffies_sleep(5);    
    }
}
// BUILTIN_THREAD(icache_prefetch, icache_prefetch, NULL);